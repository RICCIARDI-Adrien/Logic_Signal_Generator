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
	USB_CORE_DEVICE_REQUEST_ID_GET_DESCRIPTOR = 6
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

static const TUSBCoreDescriptorDevice *Pointer_USB_Core_Descriptor_Device;

//-------------------------------------------------------------------------------------------------
// Public functions
//-------------------------------------------------------------------------------------------------
void USBCoreInitialize(const void *Pointer_Descriptors)
{
	unsigned char i, Endpoints_Count = 1; // Will always be 1 or more because of the mandatory control endpoint

	// Disable eye test pattern, disable the USB OE monitoring signal, enable the on-chip pull-up, select the full-speed device mode, TODO for now disable ping-pong buffers, this will be a further optimization
	UCFG = 0x14;

	// Enable the packet transfer
	UCON = 0;

	// Keep access to the various USB descriptors
	Pointer_USB_Core_Descriptor_Device = Pointer_Descriptors; // TODO

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
	UIE = 0x09; // Enable the Transaction Complete and the Reset interrupts
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
	memcpy((void *) Pointer_Endpoint_Descriptor->In_Descriptor.Pointer_Address, Pointer_Data, Data_Size);
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
	volatile unsigned char *Pointer_Endpoint_Buffer;
	volatile TUSBCoreDeviceRequest *Pointer_Device_Request;

	LOG(USB_CORE_IS_LOGGING_ENABLED, "\033[33m--- Entering USB handler ---\033[0m");

	// Display low level debugging information
	#if USB_CORE_IS_LOGGING_ENABLED
	{
		volatile unsigned char *Pointer_Endpoint_Register;

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
		return;
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
							unsigned char Descriptor_Type, Descriptor_Index;

							// Retrieve the request parameters
							Descriptor_Type = Pointer_Device_Request->wValue >> 8;
							Descriptor_Index = (unsigned char) Pointer_Device_Request->wValue;
							LOG(USB_CORE_IS_LOGGING_ENABLED, "Host is asking for %d bytes of the descriptor of type %d and index %d.", Pointer_Device_Request->wLength, Descriptor_Type, Descriptor_Index);

							USBCorePrepareForInTransfer(0, (void *) Pointer_USB_Core_Descriptor_Device, sizeof(TUSBCoreDescriptorDevice), 1);
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

	// Clear the interrupt flag
	PIR3bits.USBIF = 0;
}
