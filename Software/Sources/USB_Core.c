/** @file USB_Core.c
 * See USB_Core.h
 * @author Adrien RICCIARDI
 */
#include <Log.h>
#include <USB_Core.h>
#include <xc.h>

//-------------------------------------------------------------------------------------------------
// Private constants
//-------------------------------------------------------------------------------------------------
/** Set to 1 to enable the log messages, set to 0 to disable them. */
#define USB_CORE_IS_LOGGING_ENABLED 1

#define USB_CORE_DEVICE_REQUEST_TYPE_MASK_TYPE 0x60
#define USB_CORE_DEVICE_REQUEST_TYPE_VALUE_TYPE_STANDARD (0 << 5)
#define USB_CORE_DEVICE_REQUEST_TYPE_VALUE_TYPE_CLASS (1 << 5)
#define USB_CORE_DEVICE_REQUEST_TYPE_VALUE_TYPE_VENDOR (2 << 5)

#define USB_CORE_DEVICE_REQUEST_TYPE_MASK_RECIPIENT 0x1F
#define USB_CORE_DEVICE_REQUEST_TYPE_VALUE_RECIPIENT_DEVICE 0
#define USB_CORE_DEVICE_REQUEST_TYPE_VALUE_RECIPIENT_INTERFACE 1
#define USB_CORE_DEVICE_REQUEST_TYPE_VALUE_RECIPIENT_ENDPOINT 2
#define USB_CORE_DEVICE_REQUEST_TYPE_VALUE_RECIPIENT_OTHER 3

//-------------------------------------------------------------------------------------------------
// Private types
//-------------------------------------------------------------------------------------------------
/** All supported USB packet ID types (see USB 2.0 specifications table 8-1). */
typedef enum : unsigned char
{
	USB_CORE_PACKET_IDENTIFIER_TYPE_TOKEN_OUT = 0x01,
	USB_CORE_PACKET_IDENTIFIER_TYPE_HANDSHAKE_ACK = 0x02,
	USB_CORE_PACKET_IDENTIFIER_TYPE_TOKEN_SETUP = 0x0D
} TUSBCorePacketIdentifierType;

/** An SIE endpoint descriptor, applicable to a single-direction hardware endpoint. */
typedef struct
{
	union
	{
		/** Map the status register content when the microcontroller has initialized it and gives to the SIE. */
		struct TUSBCoreBufferDescriptorStatusToPeripheral
		{
			unsigned char Bytes_Count_High : 2;
			unsigned char Is_Buffer_Stalled : 1;
			unsigned char Is_Data_Toggle_Synchronized_Enabled : 1;
			unsigned char Reserved : 2;
			unsigned char Data_Toggle_Synchronization : 1;
			unsigned char Is_Owned_By_Peripheral : 1;
		} Status_To_Peripheral;
		/** Map the status register content when the SIE has written to it. */
		struct TUSBCoreBufferDescriptorStatusFromPeripheral
		{
			unsigned char Bytes_Count_High : 2;
			unsigned char PID : 4;
			unsigned char Reserved : 1;
			unsigned char Is_Owned_By_Peripheral : 1;
		} Status_From_Peripheral;
		/** Allow to access directly to the register content. */
		unsigned char Status;
	};
	unsigned char Bytes_Count;
	volatile unsigned char *Pointer_Address; // The compiler manual tells that this type of pointer is always 2-byte long
} __attribute__((packed)) TUSBCoreBufferDescriptor;

/** Consider than the in and out directions are enabled for all endpoints, so always map them both into the SIE endpoint descriptors memory. */
typedef struct
{
	volatile TUSBCoreBufferDescriptor Out_Descriptor; //!< A transfer from the host to the device.
	volatile TUSBCoreBufferDescriptor In_Descriptor; //!< A transfer from the device to the host.
} __attribute__((packed)) TUSBCoreEndpointBufferDescriptor;

//-------------------------------------------------------------------------------------------------
// Private variables
//-------------------------------------------------------------------------------------------------
/** Reserve the space for all possible buffer descriptors, even if they are not used (TODO this is hardcoded for now). */
static volatile TUSBCoreEndpointBufferDescriptor USB_Core_Endpoint_Descriptors[32] __at(0x400);

/** Reserve the space for the USB buffers. */
static volatile unsigned char USB_Core_Buffers[USB_CORE_HARDWARE_ENDPOINTS_COUNT * USB_CORE_ENDPOINT_PACKETS_SIZE] __at(0x500);

/** Allow a direct access to the device descriptor everywhere in the module. */
static const TUSBCoreDescriptorDevice *Pointer_USB_Core_Device_Descriptor;

//-------------------------------------------------------------------------------------------------
// Private functions
//-------------------------------------------------------------------------------------------------
/** Stall the IN buffer descriptor of the specified endpoint.
 * @param Endpoint_ID The endpoint to stall.
 * @note This is mostly used with the control endpoint to tell that a feature is not supported.
 */
static void USBCoreStallEndpoint(unsigned char Endpoint_ID)
{
	volatile TUSBCoreEndpointBufferDescriptor *Pointer_Endpoint_Descriptor;

	// Cache the buffer descriptor access
	Pointer_Endpoint_Descriptor = &USB_Core_Endpoint_Descriptors[Endpoint_ID];

	// Get immediate ownership of the endpoint, do not wait for it to be returned by the SIE (otherwise, this function would block if multiple STALL need to be issued in a row)
	Pointer_Endpoint_Descriptor->In_Descriptor.Status = 0;
	Pointer_Endpoint_Descriptor->In_Descriptor.Status_To_Peripheral.Is_Buffer_Stalled = 1;
	Pointer_Endpoint_Descriptor->In_Descriptor.Status_From_Peripheral.Is_Owned_By_Peripheral = 1;
}

/** Send to the host the expected amount of configuration data. This function takes care of preparing the appropriate control pipe IN transfer.
 * @param Configuration_Index The configuration to obtain information from.
 * @param Length The amount of configuration bytes requested by the host.
 */
static inline void USBCoreProcessGetConfigurationDescriptor(unsigned char Configuration_Index, unsigned char Length)
{
	volatile unsigned char *Pointer_Endpoint_Descriptor_Buffer = USB_Core_Endpoint_Descriptors[0].In_Descriptor.Pointer_Address; // Cache the control descriptor buffer address
	unsigned char Count;
	const TUSBCoreDescriptorConfiguration *Pointer_Configuration_Descriptor;

	// Check the correctness of some values
	// There must be at least one configuration
	Count = Pointer_USB_Core_Device_Descriptor->bNumConfigurations;
	if (Count == 0)
	{
		LOG(USB_CORE_IS_LOGGING_ENABLED, "Error : the device descriptor has 0 configuration, which is not allowed, aborting.");
		return;
	}
	if (Configuration_Index >= Count)
	{
		LOG(USB_CORE_IS_LOGGING_ENABLED, "Error : an out-of-bounds configuration index %u has been requested (the device descriptor has %u configurations), aborting.", Configuration_Index, Count);
		return;
	}

	// Clamp the requested total length to the one of a packet
	if (Length > USB_CORE_ENDPOINT_PACKETS_SIZE)
	{
		LOG(USB_CORE_IS_LOGGING_ENABLED, "Limiting the requested size of %u bytes to the maximum configured %u bytes.", Length, USB_CORE_ENDPOINT_PACKETS_SIZE);
		Length = USB_CORE_ENDPOINT_PACKETS_SIZE;
	}

	// Find the requested configuration
	Pointer_Configuration_Descriptor = &Pointer_USB_Core_Device_Descriptor->Pointer_Configurations[Configuration_Index];
	LOG(USB_CORE_IS_LOGGING_ENABLED, "Found the configuration descriptor %u.", Configuration_Index);

	// Always start from the configuration descriptor itself
	USB_CORE_MEMCPY(Pointer_Endpoint_Descriptor_Buffer, Pointer_Configuration_Descriptor, USB_CORE_DESCRIPTOR_SIZE_CONFIGURATION);

	// Append the interfaces if asked to
	if (Length > USB_CORE_DESCRIPTOR_SIZE_CONFIGURATION)
	{
		Pointer_Endpoint_Descriptor_Buffer += USB_CORE_DESCRIPTOR_SIZE_CONFIGURATION;
		LOG(USB_CORE_IS_LOGGING_ENABLED, "The configuration descriptor has %u interfaces, its total length is %u bytes.", Pointer_Configuration_Descriptor->bNumInterfaces, Pointer_Configuration_Descriptor->wTotalLength);

		// Make sure only no more bytes than contained in the descriptor are transmitted
		if (Length > Pointer_Configuration_Descriptor->wTotalLength)
		{
			Length = (unsigned char) Pointer_Configuration_Descriptor->wTotalLength;
			LOG(USB_CORE_IS_LOGGING_ENABLED, "The requested length is greater than the descriptor length, adjusting the requested length.");
		}

		// Append all interface descriptors
		USB_CORE_MEMCPY(Pointer_Endpoint_Descriptor_Buffer, Pointer_Configuration_Descriptor->Pointer_Interfaces_Data, Length - USB_CORE_DESCRIPTOR_SIZE_CONFIGURATION);
	}

	USBCorePrepareForInTransfer(0, NULL, Length, 1);
}

/** Send to the host the expected amount of string data. This function takes care of preparing the appropriate control pipe IN transfer.
 * @param String_Index The string to obtain information from.
 * @param Length The amount of string bytes requested by the host.
 */
static inline void USBCoreProcessGetStringDescriptor(unsigned char String_Index, unsigned char Length)
{
	const unsigned char STRING_DESCRIPTOR_HEADER_SIZE = 2;
	volatile unsigned char *Pointer_Endpoint_Descriptor_Buffer = USB_Core_Endpoint_Descriptors[0].In_Descriptor.Pointer_Address; // Cache the control descriptor buffer address
	const TUSBCoreDescriptorString *Pointer_String_Descriptor;

	// Is this string descriptor existing ?
	if (String_Index >= Pointer_USB_Core_Device_Descriptor->String_Descriptors_Count) // TODO compute the amount of strings in the whole descriptors and do more secure checks
	{
		LOG(USB_CORE_IS_LOGGING_ENABLED, "Error : an out-of-bounds string index %u has been requested (the device descriptor has %u string descriptors), aborting.", String_Index, Pointer_USB_Core_Device_Descriptor->String_Descriptors_Count);
		return;
	}

	// Clamp the requested total length to the one of a packet
	if (Length > USB_CORE_ENDPOINT_PACKETS_SIZE)
	{
		LOG(USB_CORE_IS_LOGGING_ENABLED, "Limiting the requested size of %u bytes to the maximum configured %u bytes.", Length, USB_CORE_ENDPOINT_PACKETS_SIZE);
		Length = USB_CORE_ENDPOINT_PACKETS_SIZE;
	}

	// Cache access to the string descriptor
	Pointer_String_Descriptor = &Pointer_USB_Core_Device_Descriptor->Pointer_Strings[String_Index];
	if (Pointer_String_Descriptor->bLength < Length) Length = Pointer_String_Descriptor->bLength; // Clamp the size to the descriptor one, otherwise use the size asked by the host
	LOG(USB_CORE_IS_LOGGING_ENABLED, "Selecting the string descriptor %u of size %u bytes (transmitting %u bytes).", String_Index, Pointer_String_Descriptor->bLength, Length);

	// Start with the "header" of the descriptor
	USB_CORE_MEMCPY(Pointer_Endpoint_Descriptor_Buffer, Pointer_String_Descriptor, STRING_DESCRIPTOR_HEADER_SIZE);
	Pointer_Endpoint_Descriptor_Buffer += STRING_DESCRIPTOR_HEADER_SIZE;
	// Append the string data
	USB_CORE_MEMCPY(Pointer_Endpoint_Descriptor_Buffer, Pointer_String_Descriptor->Pointer_Data, Length - STRING_DESCRIPTOR_HEADER_SIZE);

	USBCorePrepareForInTransfer(0, NULL, Length, 1);
}

//-------------------------------------------------------------------------------------------------
// Public functions
//-------------------------------------------------------------------------------------------------
void USBCoreInitialize(const TUSBCoreDescriptorDevice *Pointer_Device_Descriptor)
{
	volatile TUSBCoreEndpointBufferDescriptor *Pointer_Endpoint_Descriptor;
	unsigned char i, Endpoints_Count;
	volatile unsigned char *Pointer_Endpoint_Data_Buffer, *Pointer_Endpoint_Register;
	TUSBCoreHardwareEndpointConfiguration *Pointer_Endpoint_Hardware_Configuration;

	// Make sure there are enough alloted hardware endpoints
	Endpoints_Count = Pointer_Device_Descriptor->Hardware_Endpoints_Count;
	if (Endpoints_Count > USB_CORE_HARDWARE_ENDPOINTS_COUNT)
	{
		LOG(USB_CORE_IS_LOGGING_ENABLED, "Error : %u hardware endpoints are configured while only %u have been enabled (see USB_CORE_HARDWARE_ENDPOINTS_COUNT).", Endpoints_Count, USB_CORE_HARDWARE_ENDPOINTS_COUNT);
		return;
	}
	LOG(USB_CORE_IS_LOGGING_ENABLED, "There are %u hardware endpoints to configure.", Endpoints_Count);

	// Disable eye test pattern, disable the USB OE monitoring signal, enable the on-chip pull-up, select the full-speed device mode, TODO for now disable ping-pong buffers, this will be a further optimization
	UCFG = 0x14;

	// Enable the packet transfer
	UCON = 0;

	// Keep access to the various USB descriptors
	Pointer_USB_Core_Device_Descriptor = Pointer_Device_Descriptor;

	// Configure the buffer descriptors
	Pointer_Endpoint_Descriptor = USB_Core_Endpoint_Descriptors;
	Pointer_Endpoint_Data_Buffer = USB_Core_Buffers;
	Pointer_Endpoint_Hardware_Configuration = Pointer_USB_Core_Device_Descriptor->Pointer_Hardware_Endpoints_Configuration;
	Pointer_Endpoint_Register = &UEP0;
	for (i = 0; i < Endpoints_Count; i++)
	{
		// Assign the data buffers
		Pointer_Endpoint_Descriptor->Out_Descriptor.Pointer_Address = Pointer_Endpoint_Data_Buffer;
		Pointer_Endpoint_Hardware_Configuration->Out_Transfer_Callback_Data.Pointer_OUT_Data_Buffer = (unsigned char *) Pointer_Endpoint_Data_Buffer;
		Pointer_Endpoint_Data_Buffer += USB_CORE_ENDPOINT_PACKETS_SIZE;
		Pointer_Endpoint_Descriptor->In_Descriptor.Pointer_Address = Pointer_Endpoint_Data_Buffer;
		Pointer_Endpoint_Data_Buffer += USB_CORE_ENDPOINT_PACKETS_SIZE;

		// Make sure all endpoints belong to the MCU before booting
		Pointer_Endpoint_Descriptor->Out_Descriptor.Status = 0;
		Pointer_Endpoint_Descriptor->In_Descriptor.Status = 0;

		// Assign the endpoint ID, which is useful inside the endpoint callback
		Pointer_Endpoint_Hardware_Configuration->Out_Transfer_Callback_Data.Endpoint_ID = i;

		// Configure the hardware endpoint
		*Pointer_Endpoint_Register = 0x18 | Pointer_Endpoint_Hardware_Configuration->Enabled_Directions; // Enable endpoint handshake, disable control transfers

		// Make sure that the endpoint can receive a packet (all host transactions start with a synchronization value of 0)
		USBCorePrepareForOutTransfer(i, 0);

		Pointer_Endpoint_Register++;
		Pointer_Endpoint_Descriptor++;
		Pointer_Endpoint_Hardware_Configuration++;
	}
	// Ensure that the endpoint 0, used as the control endpoint, is always correctly configured
	UEP0 = 0x16; // Enable endpoint handshake, allow control transfers, enable the endpoint OUT and IN directions

	// Configure the interrupts
	PIE3bits.USBIE = 1; // Enable the USB peripheral global interrupt
	UIE = 0x29; // Enable the STALL Handshake, the Transaction Complete and the Reset interrupts
	IPR3bits.USBIP = 1; // Set the USB interrupt as high priority

	// Enable the USB module and attach the device to the USB bus
	UCONbits.USBEN = 1;

	// Use the USB bus precise timings to keep the microcontroller clock synchronized
	ACTCON = 0xD0; // Enable the active clock tuning module, allow the module to automatically update the OSCTUNE register, use the USB host clock as reference
}

void USBCorePrepareForOutTransfer(unsigned char Endpoint_ID, unsigned char Is_Data_1_Synchronization)
{
	// Cache the buffer descriptor access
	volatile TUSBCoreEndpointBufferDescriptor *Pointer_Endpoint_Descriptor = &USB_Core_Endpoint_Descriptors[Endpoint_ID];

	// Wait for any transfer concerning the endpoint to finish
	while (Pointer_Endpoint_Descriptor->Out_Descriptor.Status_From_Peripheral.Is_Owned_By_Peripheral);

	// Allow the maximum amount of data to be received
	Pointer_Endpoint_Descriptor->Out_Descriptor.Bytes_Count = USB_CORE_ENDPOINT_PACKETS_SIZE;
	Pointer_Endpoint_Descriptor->Out_Descriptor.Status = 0; // Clear all previous settings
	Pointer_Endpoint_Descriptor->Out_Descriptor.Status_To_Peripheral.Is_Data_Toggle_Synchronized_Enabled = 1;
	Pointer_Endpoint_Descriptor->Out_Descriptor.Status_To_Peripheral.Data_Toggle_Synchronization = Is_Data_1_Synchronization;
	Pointer_Endpoint_Descriptor->Out_Descriptor.Status_To_Peripheral.Is_Owned_By_Peripheral = 1; // Give the endpoint OUT buffer ownership to the USB peripheral
}

void USBCorePrepareForInTransfer(unsigned char Endpoint_ID, void *Pointer_Data, unsigned char Data_Size, unsigned char Is_Data_1_Synchronization)
{
	volatile TUSBCoreEndpointBufferDescriptor *Pointer_Endpoint_Descriptor;

	// TODO what to do with a size higher than 64 bytes ?

	// Cache the buffer descriptor access
	Pointer_Endpoint_Descriptor = &USB_Core_Endpoint_Descriptors[Endpoint_ID];

	// Wait for any transfer concerning the endpoint to finish
	while (Pointer_Endpoint_Descriptor->In_Descriptor.Status_From_Peripheral.Is_Owned_By_Peripheral);

	// Copy the data to the USB RAM
	if (Pointer_Data != NULL) USB_CORE_MEMCPY(Pointer_Endpoint_Descriptor->In_Descriptor.Pointer_Address, Pointer_Data, Data_Size);
	Pointer_Endpoint_Descriptor->In_Descriptor.Bytes_Count = Data_Size;

	// Configure the transfer settings
	Pointer_Endpoint_Descriptor->In_Descriptor.Status = 0; // Clear all previous settings
	Pointer_Endpoint_Descriptor->In_Descriptor.Status_To_Peripheral.Is_Data_Toggle_Synchronized_Enabled = 1;
	Pointer_Endpoint_Descriptor->In_Descriptor.Status_To_Peripheral.Data_Toggle_Synchronization = Is_Data_1_Synchronization;
	Pointer_Endpoint_Descriptor->In_Descriptor.Status_To_Peripheral.Is_Owned_By_Peripheral = 1; // Give the endpoint IN buffer ownership to the USB peripheral
}

void USBCoreInterruptHandler(void)
{
	static unsigned char Device_Address = 0; // Keep the assigned address to set it in a later interrupt
	unsigned char Endpoint_ID, Is_In_Transfer;
	volatile TUSBCoreEndpointBufferDescriptor *Pointer_Endpoint_Descriptor;
	volatile unsigned char *Pointer_Endpoint_Buffer, *Pointer_Endpoint_Register;
	volatile TUSBCoreDeviceRequest *Pointer_Device_Request;
	TUSBCoreHardwareEndpointConfiguration *Pointer_Hardware_Endpoints_Configuration;
	TUSBCorePacketIdentifierType Packet_Identifier_Type;

	// Clear the main USB interrupt flag at the beginning, because this flag needs to be cleared before the transfer complete one is cleared, otherwise a transfer stored in the USTAT FIFO might be lost.
	// When TRNIF is cleared and there is a transfer in the FIFO, the USBIF flag is reasserted pretty soon. That's why the latter flag needs to be cleared first.
	// There is no issue of USB interrupt handler re-entrancy because the interrupts are disabled until the handler returns
	PIR3bits.USBIF = 0;

	LOG(USB_CORE_IS_LOGGING_ENABLED, "\033[33m--- Entering USB handler ---\033[0m");

	// Cache the involved endpoint information
	Endpoint_ID = USTAT >> 3;
	Is_In_Transfer = USTATbits.DIR; // Determine the transfer direction
	Pointer_Endpoint_Descriptor = &USB_Core_Endpoint_Descriptors[Endpoint_ID]; // Cache the endpoint access

	// Display low level debugging information
	LOG_BEGIN_SECTION(USB_CORE_IS_LOGGING_ENABLED)
	{
		const char *Pointer_String_Packet_Identifier;

		// Display the fired interrupts
		printf("Status interrupts register : 0x%02X", UIR);
		if (UIRbits.SOFIF) printf(" SOF");
		if (UIRbits.STALLIF) printf(" STALL");
		if (UIRbits.IDLEIF) printf(" IDLE");
		if (UIRbits.TRNIF) printf(" TRANSCOMP");
		if (UIRbits.ACTVIF) printf(" BUSACT");
		if (UIRbits.UERRIF) printf(" USBERR");
		if (UIRbits.URSTIF) printf(" RESET");
		printf(".\r\n");

		// Display any error that occurred
		if (UEIR != 0)
		{
			printf("An error was detected, UEIR = 0x%02X.\r\n", UEIR);
			UEIR = 0; // Clear all errors to see the next ones
		}

		// Tell if a SETUP packet disabled the SIE
		if (UCONbits.PKTDIS) printf("USB packet processing is disabled (PKTDIS).\r\n");

		// Display the last endpoint activity
		printf("Last endpoint ID : %u, transaction type : %s.\r\n", Endpoint_ID, Is_In_Transfer ? "IN" : "OUT");

		// Show the received packet (OUT) type
		if (!Is_In_Transfer)
		{
			switch (Pointer_Endpoint_Descriptor->Out_Descriptor.Status_From_Peripheral.PID)
			{
				case USB_CORE_PACKET_IDENTIFIER_TYPE_TOKEN_OUT:
					Pointer_String_Packet_Identifier = "token OUT";
					break;
				case USB_CORE_PACKET_IDENTIFIER_TYPE_HANDSHAKE_ACK:
					Pointer_String_Packet_Identifier = "handshake ACK";
					break;
				case USB_CORE_PACKET_IDENTIFIER_TYPE_TOKEN_SETUP:
					Pointer_String_Packet_Identifier = "token SETUP";
					break;
				default:
					Pointer_String_Packet_Identifier = "\033[31munknown\033[0m";
					break;
			}
			printf("Received Packet Identifier (PID) : %s.\r\n", Pointer_String_Packet_Identifier);
		}

		// Tell whether the endpoint is stalled by the host
		Pointer_Endpoint_Register = &UEP0 + Endpoint_ID;
		if (*Pointer_Endpoint_Register & 0x01) printf("The endpoint is stalled.\r\n");
	}
	LOG_END_SECTION()

	// Discard every other event when the device has been reset
	if (UIRbits.URSTIF)
	{
		LOG(USB_CORE_IS_LOGGING_ENABLED, "Detected a Reset condition, starting enumeration process.");
		UIRbits.URSTIF = 0; // Clear the interrupt flag
		return;
	}

	// Re-enable a stalled endpoint upon reception of a stall handshake
	if (UIRbits.STALLIF)
	{
		// The endpoint stall indication needs to be cleared by software
		LOG(USB_CORE_IS_LOGGING_ENABLED, "Received the %s endpoint %u stall handshake, clearing the endpoint stall condition.", Is_In_Transfer ? "IN" : "OUT", Endpoint_ID);
		Pointer_Endpoint_Register = &UEP0 + Endpoint_ID;
		*Pointer_Endpoint_Register &= 0xFE;

		// Return the IN buffer descriptor to the microcontroller, otherwise it stays owned by the SIE indefinitely
		Pointer_Endpoint_Descriptor->In_Descriptor.Status = 0;

		// Clear the interrupt flag
		UIRbits.STALLIF = 0;
		return;
	}

	// Manage data transmission and reception
	if (UIRbits.TRNIF)
	{
		// Cache the endpoint data buffer access
		if (Is_In_Transfer) Pointer_Endpoint_Buffer = Pointer_Endpoint_Descriptor->In_Descriptor.Pointer_Address;
		else Pointer_Endpoint_Buffer = Pointer_Endpoint_Descriptor->Out_Descriptor.Pointer_Address;

		// IN transfer
		if (Is_In_Transfer)
		{
			LOG(USB_CORE_IS_LOGGING_ENABLED, "Sent a %u-byte packet from endpoint %u.", Pointer_Endpoint_Descriptor->In_Descriptor.Bytes_Count, Endpoint_ID);

			// Assign the device address only when the ACK of the SET ADDRESS command has been transmitted on the default address 0
			if (Device_Address != 0)
			{
				UADDR = Device_Address;
				Device_Address = 0;
			}
			else
			{
				LOG(USB_CORE_IS_LOGGING_ENABLED, "An IN transfer is completed, calling the corresponding callback (if any).");
				// Call the corresponding callback
				Pointer_Hardware_Endpoints_Configuration = &Pointer_USB_Core_Device_Descriptor->Pointer_Hardware_Endpoints_Configuration[Endpoint_ID];
				if (Pointer_Hardware_Endpoints_Configuration->In_Transfer_Callback != NULL) Pointer_Hardware_Endpoints_Configuration->In_Transfer_Callback(Endpoint_ID);
			}
		}
		// OUT or SETUP transfer
		else
		{
			// Display the received packet data
			LOG_BEGIN_SECTION(USB_CORE_IS_LOGGING_ENABLED)
			{
				unsigned char i, Bytes_Count;

				Bytes_Count = Pointer_Endpoint_Descriptor->Out_Descriptor.Bytes_Count;
				LOG(USB_CORE_IS_LOGGING_ENABLED, "Received a %u-byte packet on endpoint %u : ", Bytes_Count, Endpoint_ID);
				for (i = 0; i < Bytes_Count; i++) printf("0x%02X ", Pointer_Endpoint_Buffer[i]);
				puts("\r");
			}
			LOG_END_SECTION()

			// Handle the request according to its type
			Packet_Identifier_Type = Pointer_Endpoint_Descriptor->Out_Descriptor.Status_From_Peripheral.PID;

			// Host sending a normal OUT
			if (Packet_Identifier_Type == USB_CORE_PACKET_IDENTIFIER_TYPE_TOKEN_OUT)
			{
				LOG(USB_CORE_IS_LOGGING_ENABLED, "Decoded as a normal OUT packet, calling the corresponding endpoint callback.");

				// Call the corresponding callback
				Pointer_Hardware_Endpoints_Configuration = &Pointer_USB_Core_Device_Descriptor->Pointer_Hardware_Endpoints_Configuration[Endpoint_ID];
				Pointer_Hardware_Endpoints_Configuration->Out_Transfer_Callback_Data.Data_Size = Pointer_Endpoint_Descriptor->Out_Descriptor.Bytes_Count;
				Pointer_Hardware_Endpoints_Configuration->Out_Transfer_Callback(&Pointer_Hardware_Endpoints_Configuration->Out_Transfer_Callback_Data);
			}
			// Host acknowledging an IN transfer
			else if (Packet_Identifier_Type == USB_CORE_PACKET_IDENTIFIER_TYPE_HANDSHAKE_ACK) LOG(USB_CORE_IS_LOGGING_ENABLED, "Received a handshake ACK from the host.");
			// Host sending a SETUP request
			else if (Packet_Identifier_Type == USB_CORE_PACKET_IDENTIFIER_TYPE_TOKEN_SETUP)
			{
				// Manage the standard setup requests
				Pointer_Device_Request = (volatile TUSBCoreDeviceRequest *) Pointer_Endpoint_Buffer;
				if ((Pointer_Device_Request->bmRequestType & USB_CORE_DEVICE_REQUEST_TYPE_MASK_TYPE) == USB_CORE_DEVICE_REQUEST_TYPE_VALUE_TYPE_STANDARD)
				{
					LOG(USB_CORE_IS_LOGGING_ENABLED, "Decoded as a standard setup device request.");

					switch (Pointer_Device_Request->bRequest)
					{
						case USB_CORE_DEVICE_REQUEST_ID_SET_ADDRESS:
							// Keep the address to set it after this SETUP request has been fully serviced on the current default address 0
							Device_Address = (unsigned char) Pointer_Device_Request->wValue;
							LOG(USB_CORE_IS_LOGGING_ENABLED, "Host is setting the device address to 0x%02X.", Device_Address);

							// Send back an empty packet to acknowledge the address setting (still using the default address 0)
							USBCorePrepareForInTransfer(0, NULL, 0, 1);
							USBCorePrepareForOutTransfer(0, 0); // Re-enable packets reception
							break;

						case USB_CORE_DEVICE_REQUEST_ID_GET_DESCRIPTOR:
						{
							TUSBCoreDescriptorType Descriptor_Type;
							unsigned char Descriptor_Index;

							// Retrieve the request parameters
							Descriptor_Type = Pointer_Device_Request->wValue >> 8;
							Descriptor_Index = (unsigned char) Pointer_Device_Request->wValue;
							LOG(USB_CORE_IS_LOGGING_ENABLED, "Host is asking for %u bytes of the descriptor of type %u and index %u.", Pointer_Device_Request->wLength, Descriptor_Type, Descriptor_Index);

							switch (Descriptor_Type)
							{
								case USB_CORE_DESCRIPTOR_TYPE_CONFIGURATION:
									USBCoreProcessGetConfigurationDescriptor(Descriptor_Index, (unsigned char) Pointer_Device_Request->wLength);
									USBCorePrepareForOutTransfer(0, 0); // Re-enable packets reception
									break;

								case USB_CORE_DESCRIPTOR_TYPE_STRING:
									USBCoreProcessGetStringDescriptor(Descriptor_Index, (unsigned char) Pointer_Device_Request->wLength);
									USBCorePrepareForOutTransfer(0, 0); // Re-enable packets reception
									break;

								case USB_CORE_DESCRIPTOR_TYPE_DEVICE:
									LOG(USB_CORE_IS_LOGGING_ENABLED, "Selecting the device descriptor.");
									USBCorePrepareForInTransfer(0, (void *) Pointer_USB_Core_Device_Descriptor, USB_CORE_DESCRIPTOR_SIZE_DEVICE, 1);
									USBCorePrepareForOutTransfer(0, 0); // Re-enable packets reception
									break;

								case USB_CORE_DESCRIPTOR_TYPE_DEVICE_QUALIFIER:
									LOG(USB_CORE_IS_LOGGING_ENABLED, "Tell the host that the device qualifier descriptor is not supported.");
									// Stall the control endpoint to tell that the device does not support high speed (see USB2.0 spec chapter 9.2.7)
									USBCoreStallEndpoint(0);
									USBCorePrepareForOutTransfer(0, 0); // Re-enable packets reception
									break;

								default:
									LOG(USB_CORE_IS_LOGGING_ENABLED, "Unsupported descriptor type.");
									break;
							}
							break;
						}

						case USB_CORE_DEVICE_REQUEST_ID_SET_CONFIGURATION:
						{
							LOG(USB_CORE_IS_LOGGING_ENABLED, "The host is setting the configuration with value %u (note that only the first configuration is supported for now).", (unsigned char) Pointer_Device_Request->wValue);
							USBCorePrepareForInTransfer(0, NULL, 0, 1); // Send back an empty packet to acknowledge the configuration setting
							USBCorePrepareForOutTransfer(0, 0); // Re-enable packets reception
							break;
						}
					}
				}
				// This is a class or vendor request, forward it to the class handler
				else
				{
					if ((Pointer_Device_Request->bmRequestType & USB_CORE_DEVICE_REQUEST_TYPE_MASK_TYPE) == USB_CORE_DEVICE_REQUEST_TYPE_VALUE_TYPE_CLASS) LOG(USB_CORE_IS_LOGGING_ENABLED, "Decoded as a class request (request : 0x%02X, value = 0x%04X, index = 0x%04X, length = 0x%04X), calling the corresponding callback.", Pointer_Device_Request->bRequest, Pointer_Device_Request->wValue, Pointer_Device_Request->wIndex, Pointer_Device_Request->wLength);

					// Call the corresponding callback
					Pointer_Hardware_Endpoints_Configuration = &Pointer_USB_Core_Device_Descriptor->Pointer_Hardware_Endpoints_Configuration[Endpoint_ID];
					Pointer_Hardware_Endpoints_Configuration->Out_Transfer_Callback_Data.Data_Size = Pointer_Endpoint_Descriptor->Out_Descriptor.Bytes_Count;
					Pointer_Hardware_Endpoints_Configuration->Out_Transfer_Callback(&Pointer_Hardware_Endpoints_Configuration->Out_Transfer_Callback_Data);
				}

				// When a setup transfer is received, the SIE disables packets processing, so re-enable it now
				UCONbits.PKTDIS = 0;
			}
		}
		// Clear the interrupt flag
		UIRbits.TRNIF = 0;
	}
}
