/** @file USB_Core.c
 * See USB_Core.h
 * @author Adrien RICCIARDI
 */
#include <Log.h>
#include <string.h>
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

/** All supported USB packet ID types (see USB 2.0 specifications table 8.1). */
#define USB_CORE_PACKET_ID_TYPE_SETUP 0x0D

//-------------------------------------------------------------------------------------------------
// Private types
//-------------------------------------------------------------------------------------------------
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

/** All supported device request IDs. */
typedef enum : unsigned char
{
	USB_CORE_DEVICE_REQUEST_ID_SET_ADDRESS = 5,
	USB_CORE_DEVICE_REQUEST_ID_GET_DESCRIPTOR = 6,
	USB_CORE_DEVICE_REQUEST_ID_SET_CONFIGURATION = 9
} TUSBCoreDeviceRequestID;

/** The standard format a an USB device request. */
typedef struct
{
	unsigned char bmRequestType;
	TUSBCoreDeviceRequestID bRequest;
	unsigned short wValue;
	unsigned short wIndex;
	unsigned short wLength;
} __attribute__((packed)) TUSBCoreDeviceRequest;

//-------------------------------------------------------------------------------------------------
// Private variables
//-------------------------------------------------------------------------------------------------
/** Reserve the space for all possible buffer descriptors, even if they are not used (TODO this is hardcoded for now). */
static volatile TUSBCoreEndpointBufferDescriptor USB_Core_Endpoint_Descriptors[32] __at(0x400);

/** Reserve the space for the USB buffers (TODO this is hardcoded for now). */
static volatile unsigned char USB_Core_Buffers[128] __at(0x500);

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
	unsigned char Count, i;
	const TUSBCoreDescriptorConfiguration *Pointer_Configuration_Descriptor;

	// Check the correctness of some values
	// The requested total length
	if (Length > USB_CORE_ENDPOINT_PACKETS_SIZE)
	{
		LOG(USB_CORE_IS_LOGGING_ENABLED, "Error : the requested length %u is greater than the packet size %u, aborting.", Length, USB_CORE_ENDPOINT_PACKETS_SIZE);
		return;
	}
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

	// Find the requested configuration
	Pointer_Configuration_Descriptor = &Pointer_USB_Core_Device_Descriptor->Pointer_Configurations[Configuration_Index];
	LOG(USB_CORE_IS_LOGGING_ENABLED, "Found the configuration descriptor %u.", Configuration_Index);

	// Always start from the configuration descriptor itself
	memcpy((void *) Pointer_Endpoint_Descriptor_Buffer, Pointer_Configuration_Descriptor, USB_CORE_DESCRIPTOR_SIZE_CONFIGURATION);

	// Append the interfaces if asked to
	if (Length > USB_CORE_DESCRIPTOR_SIZE_CONFIGURATION)
	{
		Count = Pointer_Configuration_Descriptor->bNumInterfaces;
		Pointer_Endpoint_Descriptor_Buffer += USB_CORE_DESCRIPTOR_SIZE_CONFIGURATION;
		LOG(USB_CORE_IS_LOGGING_ENABLED, "The configuration descriptor has %u interfaces.", Count);

		// Append all interface descriptors
		for (i = 0; i < Count; i++)
		{
			memcpy((void *) Pointer_Endpoint_Descriptor_Buffer, &Pointer_Configuration_Descriptor->Pointer_Interfaces[i], USB_CORE_DESCRIPTOR_SIZE_INTERFACE);
			Pointer_Endpoint_Descriptor_Buffer += USB_CORE_DESCRIPTOR_SIZE_INTERFACE;
		}
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
	memcpy((void *) Pointer_Endpoint_Descriptor_Buffer, Pointer_String_Descriptor, STRING_DESCRIPTOR_HEADER_SIZE);
	Pointer_Endpoint_Descriptor_Buffer += STRING_DESCRIPTOR_HEADER_SIZE;
	// Append the string data
	memcpy((void *) Pointer_Endpoint_Descriptor_Buffer, Pointer_String_Descriptor->Pointer_Data, Length - STRING_DESCRIPTOR_HEADER_SIZE);

	USBCorePrepareForInTransfer(0, NULL, Length, 1);
}

//-------------------------------------------------------------------------------------------------
// Public functions
//-------------------------------------------------------------------------------------------------
void USBCoreInitialize(const TUSBCoreDescriptorDevice *Pointer_Device_Descriptor)
{
	unsigned char i, Endpoints_Count = 1; // Will always be 1 or more because of the mandatory control endpoint

	// Disable eye test pattern, disable the USB OE monitoring signal, enable the on-chip pull-up, select the full-speed device mode, TODO for now disable ping-pong buffers, this will be a further optimization
	UCFG = 0x14;

	// Enable the packet transfer
	UCON = 0;

	// Keep access to the various USB descriptors
	Pointer_USB_Core_Device_Descriptor = Pointer_Device_Descriptor;

	// TODO configure the buffer descriptors
	USB_Core_Endpoint_Descriptors[0].Out_Descriptor.Pointer_Address = USB_Core_Buffers;
	USB_Core_Endpoint_Descriptors[0].In_Descriptor.Pointer_Address = USB_Core_Buffers + 64;

	// Make sure all endpoints belong to the MCU before booting
	for (i = 0; i < Endpoints_Count; i++)
	{
		USB_Core_Endpoint_Descriptors[i].Out_Descriptor.Status = 0;
		USB_Core_Endpoint_Descriptors[i].In_Descriptor.Status = 0;
	}

	// Make sure that the control endpoint can receive a packet
	USBCorePrepareForOutTransfer(0, 0);

	// Always enable the endpoint 0, used as the control endpoint
	UEP0 = 0x16; // Enable endpoint handshake, allow control transfers, enabled endpoint input and output

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
	if (Pointer_Data != NULL) memcpy((void *) Pointer_Endpoint_Descriptor->In_Descriptor.Pointer_Address, Pointer_Data, Data_Size);
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

	LOG(USB_CORE_IS_LOGGING_ENABLED, "\033[33m--- Entering USB handler ---\033[0m");

	// Display low level debugging information
	#if USB_CORE_IS_LOGGING_ENABLED
	{
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
		Endpoint_ID = USTAT >> 3;
		Is_In_Transfer = USTATbits.DIR;
		printf("Last endpoint ID : %u, transaction type : %s.\r\n", Endpoint_ID, Is_In_Transfer ? "IN" : "OUT");

		// Tell whether the endpoint is stalled by the host
		Pointer_Endpoint_Register = &UEP0 + Endpoint_ID;
		if (*Pointer_Endpoint_Register & 0x01) printf("The endpoint is stalled.\r\n");
	}
	#endif

	// Discard every other event when the device has been reset
	if (UIRbits.URSTIF)
	{
		LOG(USB_CORE_IS_LOGGING_ENABLED, "Detected a Reset condition, starting enumeration process.");
		UIRbits.URSTIF = 0; // Clear the interrupt flag
		goto Exit;
	}

	// Re-enable a stalled endpoint upon reception of a stall handshake
	if (UIRbits.STALLIF)
	{
		// Cache the involved endpoint information
		Endpoint_ID = USTAT >> 3;
		Is_In_Transfer = USTATbits.DIR; // Determine the transfer direction
		Pointer_Endpoint_Descriptor = &USB_Core_Endpoint_Descriptors[Endpoint_ID]; // Cache the endpoint access

		// The endpoint stall indication needs to be cleared by software
		LOG(USB_CORE_IS_LOGGING_ENABLED, "Received the %s endpoint %u stall handshake, clearing the endpoint stall condition.", Is_In_Transfer ? "IN" : "OUT", Endpoint_ID);
		Pointer_Endpoint_Register = &UEP0 + Endpoint_ID;
		*Pointer_Endpoint_Register &= 0xFE;

		// Return the IN buffer descriptor to the microcontroller, otherwise it stays owned by the SIE indefinitely
		Pointer_Endpoint_Descriptor->In_Descriptor.Status = 0;

		// Clear the interrupt flag
		UIRbits.STALLIF = 0;
		goto Exit;
	}

	// Manage data transmission and reception
	if (UIRbits.TRNIF)
	{
		// Cache the involved endpoint information
		Endpoint_ID = USTAT >> 3;
		Is_In_Transfer = USTATbits.DIR; // Determine the transfer direction
		Pointer_Endpoint_Descriptor = &USB_Core_Endpoint_Descriptors[Endpoint_ID]; // Cache the endpoint access

		// Cache the endpoint data buffer access
		if (Is_In_Transfer) Pointer_Endpoint_Buffer = Pointer_Endpoint_Descriptor->In_Descriptor.Pointer_Address;
		else Pointer_Endpoint_Buffer = Pointer_Endpoint_Descriptor->Out_Descriptor.Pointer_Address;

		// IN transfer
		if (Is_In_Transfer)
		{
			#if USB_CORE_IS_LOGGING_ENABLED
				unsigned char Bytes_Count;

				Bytes_Count = Pointer_Endpoint_Descriptor->In_Descriptor.Bytes_Count;
				LOG(USB_CORE_IS_LOGGING_ENABLED, "Sent a %d-byte packet from endpoint %d.", Bytes_Count, Endpoint_ID);
			#endif

			// Assign the device address only when the ACK of the SET ADDRESS command has been transmitted on the default address 0
			if (Device_Address != 0)
			{
				UADDR = Device_Address;
				Device_Address = 0;
			}
		}
		// OUT or SETUP transfer
		else
		{
			// Display the received packet data
			#if USB_CORE_IS_LOGGING_ENABLED
				unsigned char i, Bytes_Count;

				Bytes_Count = Pointer_Endpoint_Descriptor->Out_Descriptor.Bytes_Count;
				LOG(USB_CORE_IS_LOGGING_ENABLED, "Received a %d-byte packet on endpoint %d : ", Bytes_Count, Endpoint_ID);
				for (i = 0; i < Bytes_Count; i++) printf("0x%02X ", Pointer_Endpoint_Buffer[i]);
				puts("\r");
			#endif

			// Manage the standard setup requests (TODO organize better to support other requests)
			if (Pointer_Endpoint_Descriptor->Out_Descriptor.Status_From_Peripheral.PID == USB_CORE_PACKET_ID_TYPE_SETUP) // Such requests are only addressed to the endpoint 0
			{
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
							LOG(USB_CORE_IS_LOGGING_ENABLED, "The host is setting the configuration with value %u (note that only the first configuration is supported for now).", (unsigned char) (Pointer_Device_Request->wValue >> 8));
							USBCorePrepareForInTransfer(0, NULL, 0, 1); // Send back an empty packet to acknowledge the configuration setting
							USBCorePrepareForOutTransfer(0, 0); // Re-enable packets reception
							break;
						}
					}

					// When a setup transfer is received, the SIE disables packets processing, so re-enable it now
					UCONbits.PKTDIS = 0;
				}
			}
		}

		// Clear the interrupt flag
		UIRbits.TRNIF = 0;
	}

Exit:
	// Clear the interrupt flag
	PIR3bits.USBIF = 0;
}
