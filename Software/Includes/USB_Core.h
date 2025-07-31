/** @file USB_Core.h
 * All USB-generic code not related to a specific class.
 * @author Adrien RICCIARDI
 */
#ifndef H_USB_CORE_H
#define H_USB_CORE_H

#include <xc.h>

//-------------------------------------------------------------------------------------------------
// Constants and macros
//-------------------------------------------------------------------------------------------------
/** The size in byte of any endpoint buffer (this conforms to the USB 2.0 specifications for full-speed devices). */
#define USB_CORE_ENDPOINT_PACKETS_SIZE 64

/** How many hardware endpoints to map into memory. */
#define USB_CORE_HARDWARE_ENDPOINTS_COUNT 4

/** Tell whether the USB peripheral interrupt needs to be serviced. */
#define USB_CORE_IS_INTERRUPT_FIRED() PIR3bits.USBIF // No need to check whether the interrupt is enabled in the PIE register, because the USB interrupt is always enabled

/** The USB specification release number in BCD format to use in the relevant descriptors. */
#define USB_CORE_BCD_USB_SPECIFICATION_RELEASE_NUMBER { 0x00, 0x02 }

/** The size in bytes of the device descriptor. */
#define USB_CORE_DESCRIPTOR_SIZE_DEVICE 18
/** The size in bytes of the configuration descriptor. */
#define USB_CORE_DESCRIPTOR_SIZE_CONFIGURATION 9
/** The size in bytes of a string descriptor.
 * @param Data_Size The size in bytes of the data.
 */
#define USB_CORE_DESCRIPTOR_SIZE_STRING(Data_Size) (2 + Data_Size)
/** The size in bytes of the interface descriptor. */
#define USB_CORE_DESCRIPTOR_SIZE_INTERFACE 9

/** Retrieve the amount of elements in an array.
 * @param Array The array (not a pointer on the array).
 */
#define USB_CORE_ARRAY_SIZE(Array) (sizeof(Array) / (sizeof(Array[0])))

/** The endpoint descriptor bmAttributes Transfer Type field value for an OUT direction. */
#define USB_CORE_DESCRIPTOR_ENDPOINT_ATTRIBUTE_ENDPOINT_ADDRESS_DIRECTION_OUT 0
/** The endpoint descriptor bmAttributes Transfer Type field value for an IN direction. */
#define USB_CORE_DESCRIPTOR_ENDPOINT_ATTRIBUTE_ENDPOINT_ADDRESS_DIRECTION_IN 0x80

/** The endpoint descriptor bmAttributes Transfer Type field value for Bulk. */
#define USB_CORE_DESCRIPTOR_ENDPOINT_ATTRIBUTE_TRANSFER_TYPE_BULK 2
/** The endpoint descriptor bmAttributes Transfer Type field value for Interrupt. */
#define USB_CORE_DESCRIPTOR_ENDPOINT_ATTRIBUTE_TRANSFER_TYPE_INTERRUPT 3

/** Enabled the OUT endpoint of the hardware endpoint. */
#define USB_CORE_HARDWARE_ENDPOINT_DIRECTION_OUT 0x04
/** Enabled the IN endpoint of the hardware endpoint. */
#define USB_CORE_HARDWARE_ENDPOINT_DIRECTION_IN 0x02

/** Copy data from a buffer to another, keeping the volatile qualifier and forcing the inlining.
 * @param Pointer_Destination_Buffer Where to store the copied data.
 * @param Pointer_Source_Buffer The data to copy from.
 * @param Bytes_Count How many bytes to copy.
 */
#define USB_CORE_MEMCPY(Pointer_Destination_Buffer, Pointer_Source_Buffer, Bytes_Count) \
{ \
	unsigned char i = (Bytes_Count); \
	volatile unsigned char *Pointer_Destination_Buffer_Bytes = (volatile unsigned char *) Pointer_Destination_Buffer; \
	volatile unsigned char *Pointer_Source_Buffer_Bytes = (volatile unsigned char *) Pointer_Source_Buffer; \
	while (i > 0) \
	{ \
		*Pointer_Destination_Buffer_Bytes = *Pointer_Source_Buffer_Bytes; \
		Pointer_Destination_Buffer_Bytes++; \
		Pointer_Source_Buffer_Bytes++; \
		i--; \
	} \
}

//-------------------------------------------------------------------------------------------------
// Types
//-------------------------------------------------------------------------------------------------
/** The type of an USB descriptor. Ignore the values related to high-speed. */
typedef enum : unsigned char
{
	USB_CORE_DESCRIPTOR_TYPE_DEVICE = 1,
	USB_CORE_DESCRIPTOR_TYPE_CONFIGURATION = 2,
	USB_CORE_DESCRIPTOR_TYPE_STRING = 3,
	USB_CORE_DESCRIPTOR_TYPE_INTERFACE = 4,
	USB_CORE_DESCRIPTOR_TYPE_ENDPOINT = 5,
	USB_CORE_DESCRIPTOR_TYPE_DEVICE_QUALIFIER = 6
} TUSBCoreDescriptorType;

// Device descriptor
/** The supported device class codes. */
typedef enum : unsigned char
{
	USB_CORE_DEVICE_CLASS_CODE_COMMUNICATIONS = 2 //!< CDC.
} TUSBCoreDeviceClassCode;

/** The supported device sub class codes. */
typedef enum : unsigned char
{
	USB_CORE_DEVICE_SUB_CLASS_CODE_NONE = 0
} TUSBCoreDeviceSubClassCode;

/** The supported device protocol codes. */
typedef enum : unsigned char
{
	USB_CORE_DEVICE_PROTOCOL_CODE_NONE = 0 //!< No device class-specific protocol is used.
} TUSBCoreDeviceProtocolCode;

// Interface descriptor
/** The supported interface class codes. */
typedef enum : unsigned char
{
	USB_CORE_INTERFACE_CLASS_CODE_COMMUNICATIONS = 2,
	USB_CORE_INTERFACE_CLASS_CODE_DATA_INTERFACE = 0x0A
} TUSBCoreInterfaceClassCode;

/** The supported interface sub class codes. */
typedef enum : unsigned char
{
	USB_CORE_INTERFACE_SUB_CLASS_CODE_NONE = 0,
	USB_CORE_INTERFACE_SUB_CLASS_CODE_ABSTRACT_CONTROL_MODEL = 2 //!< ACM.
} TUSBCoreInterfaceSubClassCode;

/** The supported interface protocol codes. */
typedef enum : unsigned char
{
	USB_CORE_INTERFACE_PROTOCOL_CODE_NONE = 0,
	USB_CORE_INTERFACE_PROTOCOL_CODE_ITU_V250 = 1
} TUSBCoreInterfaceProtocolCode;

/** Some language identifier codes. See the USB document named "Language Identifiers (LANGIDs)". */
typedef enum : unsigned short
{
	USB_CORE_LANGUAGE_ID_ENGLISH_UNITED_STATES = 0x0409,
	USB_CORE_LANGUAGE_ID_FRENCH_STANDARD = 0x040C
} TUSBCoreLanguageID;

/** Called when a OUT transfert is received.
 * @param Pointer_Data The hardware USB buffer containing the received data.
 * @param Data_Size The size of the received data.
 */
typedef void (*TUSBCoreHardwareEndpointOUTCallback)(volatile unsigned char *Pointer_Data, unsigned char Data_Size);

/** How to configure the microcontroller hardware USB endpoints. */
typedef struct
{
	unsigned char Enabled_Directions; //!< Configure the IN and/or the OUT endpoints.
	TUSBCoreHardwareEndpointOUTCallback Out_Transfert_Callback; //!< Called when a non-SETUP OUT transfert is received.
	volatile unsigned char *Pointer_Data_Buffer_OUT; //!< Reserved for internal use.
	volatile unsigned char *Pointer_Data_Buffer_IN; //!< Reserved for internal use.
} TUSBCoreHardwareEndpointConfiguration;

/** An USB string descriptor using the USB naming for simplicity. See the USB specifications 2.0 table 9.6.7. */
typedef struct
{
	unsigned char bLength;
	TUSBCoreDescriptorType bDescriptorType;
	const void *Pointer_Data; //!< This field is not part of the USB specification.
} __attribute__((packed)) TUSBCoreDescriptorString;

/** An USB endpoint descriptor using the USB naming for simplicity. See the USB specifications 2.0 table 9.13. */
typedef struct
{
	unsigned char bLength;
	TUSBCoreDescriptorType bDescriptorType;
	unsigned char bEndpointAddress;
	unsigned char bmAttributes;
	unsigned short wMaxPacketSize;
	unsigned char bInterval;
} __attribute__((packed)) TUSBCoreDescriptorEndpoint;

/** An USB interface descriptor using the USB naming for simplicity. See the USB specifications 2.0 table 9.12. */
typedef struct
{
	unsigned char bLength;
	TUSBCoreDescriptorType bDescriptorType;
	unsigned char bInterfaceNumber;
	unsigned char bAlternateSetting;
	unsigned char bNumEndpoints; //!< The endpoint 0, if used, is excluded from this value.
	TUSBCoreInterfaceClassCode bInterfaceClass;
	TUSBCoreInterfaceSubClassCode bInterfaceSubClass;
	TUSBCoreInterfaceProtocolCode bInterfaceProtocol;
	unsigned char iInterface;
} __attribute__((packed)) TUSBCoreDescriptorInterface;

/** An USB configuration descriptor, using the USB naming for simplicity. See the USB specifications 2.0 table 9.10. */
typedef struct
{
	unsigned char bLength;
	TUSBCoreDescriptorType bDescriptorType;
	unsigned short wTotalLength; //!< This is the combined length of the configuration, related interfaces and endpoints.
	unsigned char bNumInterfaces;
	unsigned char bConfigurationValue;
	unsigned char iConfiguration;
	unsigned char bmAttributes;
	unsigned char bMaxPower; //!< Expressed in 2mA units.
	const void *Pointer_Interfaces_Data; //!< This field is not part of the USB specification.
} __attribute__((packed)) TUSBCoreDescriptorConfiguration;

/** An USB device descriptor, using the USB naming for simplicity. */
typedef struct
{
	unsigned char bLength;
	TUSBCoreDescriptorType bDescriptorType;
	unsigned char bcdUSB[2];
	TUSBCoreDeviceClassCode bDeviceClass;
	TUSBCoreDeviceSubClassCode bDeviceSubClass;
	TUSBCoreDeviceProtocolCode bDeviceProtocol;
	unsigned char bMaxPacketSize0;
	unsigned short idVendor;
	unsigned short idProduct;
	unsigned short bcdDevice;
	unsigned char iManufacturer;
	unsigned char iProduct;
	unsigned char iSerialNumber;
	unsigned char bNumConfigurations;
	const TUSBCoreDescriptorConfiguration *Pointer_Configurations; //!< This field is not part of the USB specification.
	const TUSBCoreDescriptorString *Pointer_Strings; //!< This field is not part of the USB specification.
	unsigned char String_Descriptors_Count; //!< This field is not part of the USB specification.
	// TODO the two following fields should belong to the TUSBCoreDescriptorConfiguration struct and be configured when a configuration is selected by the host, this is too complex for the current usage so configure all endpoints at boot
	TUSBCoreHardwareEndpointConfiguration *Pointer_Hardware_Endpoints_Configuration; //!< This field is not part of the USB specification.
	unsigned char Hardware_Endpoints_Count; //!< This field is not part of the USB specification.
} __attribute__((packed)) TUSBCoreDescriptorDevice;

//-------------------------------------------------------------------------------------------------
// Functions
//-------------------------------------------------------------------------------------------------
/** Configure the USB peripheral for full-speed operations and attach the device to the bus.
 * @param Pointer_Device_Descriptor Gather all configuration settings.
 */
void USBCoreInitialize(const TUSBCoreDescriptorDevice *Pointer_Device_Descriptor);

/** Configure the specified endpoint out buffer for an upcoming reception of data from the host.
 * @param Endpoint_ID The endpoint number (any endpoint other than 0 must have been enabled in the device descriptors).
 * @param Is_Data_1_Synchronization The data synchronization value to expect from the host. Set to 0 for a DATA0 packet ID or set to 1 for a DATA1 packet ID.
 * @note The maximum endpoint packet size is always set to USB_CORE_ENDPOINT_PACKETS_SIZE.
 */
void USBCorePrepareForOutTransfer(unsigned char Endpoint_ID, unsigned char Is_Data_1_Synchronization);

/** TODO */
void USBCorePrepareForInTransfer(unsigned char Endpoint_ID, void *Pointer_Data, unsigned char Data_Size, unsigned char Is_Data_1_Synchronization);

/** Must be called from the interrupt context to handle the USB interrupt. */
void USBCoreInterruptHandler(void);

#endif
