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

//-------------------------------------------------------------------------------------------------
// Private types
//-------------------------------------------------------------------------------------------------
/** An SIE endpoint descriptor, applicable to a single-direction hardware endpoint. */
typedef struct
{
	unsigned char Status;
	unsigned char Bytes_Count;
	volatile unsigned char *Pointer_Address; // The compiler manual tells that this type of pointer is always 2-byte long
} __attribute__((packed)) TUSBCoreBufferDescriptor;

/** Consider than the in and out directions are enabled for all endpoints, so always map them both into the SIE endpoint descriptors memory. */
typedef struct
{
	TUSBCoreBufferDescriptor Out_Descriptor; //!< A transfer from the host to the device.
	TUSBCoreBufferDescriptor In_Descriptor; //!< A transfer from the device to the host.
} __attribute__((packed)) TUSBCoreEndpointBufferDescriptor;

//-------------------------------------------------------------------------------------------------
// Private variables
//-------------------------------------------------------------------------------------------------
/** Reserve the space for all possible buffer descriptors, even if they are not used (TODO this is hardcoded for now). */
static volatile TUSBCoreEndpointBufferDescriptor USB_Core_Endpoint_Descriptors[32] __at(0x400);

/** Reserve the space for the USB buffers (TODO this is hardcoded for now). */
static volatile unsigned char USB_Core_Buffers[128] __at(0x500);

//-------------------------------------------------------------------------------------------------
// Public functions
//-------------------------------------------------------------------------------------------------
void USBCoreInitialize(void)
{
	// Disable eye test pattern, disable the USB OE monitoring signal, enable the on-chip pull-up, select the full-speed device mode, TODO for now disable ping-pong buffers, this will be a further optimization
	UCFG = 0x14;

	// Enable the packet transfer
	UCON = 0;

	// Always enable the endpoint 0, used as the control endpoint
	UEP0 = 0x16; // Enable endpoint handshake, allow control transfers, enabled endpoint input and output

	// TODO configure the buffer descriptors
	USB_Core_Endpoint_Descriptors[0].Out_Descriptor.Pointer_Address = USB_Core_Buffers;
	USBCorePrepareForOutTransfer(0);

	// Configure the interrupts
	PIE3bits.USBIE = 1; // Enable the USB peripheral global interrupt
	UIE = 0x09; // Enable the Transaction Complete and the Reset interrupts
	IPR3bits.USBIP = 1; // Set the USB interrupt as high priority

	// Enable the USB module and attach the device to the USB bus
	UCONbits.USBEN = 1;
}

void USBCorePrepareForOutTransfer(unsigned char Endpoint_ID)
{
	// Cache the buffer descriptor access
	volatile TUSBCoreEndpointBufferDescriptor *Pointer_Endpoint_Descriptor = &USB_Core_Endpoint_Descriptors[Endpoint_ID];

	// Allow the maximum amount of data to be received
	Pointer_Endpoint_Descriptor->Out_Descriptor.Bytes_Count = USB_CORE_ENDPOINT_PACKETS_SIZE;
	Pointer_Endpoint_Descriptor->Out_Descriptor.Status = 0x80; // Give the endpoint OUT buffer ownership to the USB peripheral
}

void USBCoreInterruptHandler(void)
{
	unsigned char Endpoint_ID, Is_In_Transfer;
	volatile TUSBCoreEndpointBufferDescriptor *Pointer_Endpoint_Descriptor;
	volatile unsigned char *Pointer_Endpoint_Buffer;

	// Wait for the address and discard every other event when the device has been reset
	if (UIRbits.URSTIF)
	{
		LOG(USB_CORE_IS_LOGGING_ENABLED, "Detected a Reset condition, starting enumeration process.");
		UIR = 0; // Clear all interrupts to discard all other events

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

		// Display the received packet data if this is an out or a setup transfer
		#if USB_CORE_IS_LOGGING_ENABLED
		if (!Is_In_Transfer)
		{
			unsigned char i, Bytes_Count;

			Bytes_Count = Pointer_Endpoint_Descriptor->Out_Descriptor.Bytes_Count;
			LOG(USB_CORE_IS_LOGGING_ENABLED, "Received a %d-byte packet : ", Bytes_Count);
			for (i = 0; i < Bytes_Count; i++) printf("0x%02X ", Pointer_Endpoint_Buffer[i]);
			puts("\r");
		}
		#endif

		// Clear the interrupt flag
		UIRbits.TRNIF = 0;
	}

	// Clear the interrupt flag
	PIR3bits.USBIF = 0;
}
