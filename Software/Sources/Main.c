/** @file Main.c
 * Logic signal generator entry point and main loop.
 * @author Adrien RICCIARDI
 */
#include <Log.h>
#include <UART.h>
#include <USB_Communications.h>
#include <xc.h>

//-------------------------------------------------------------------------------------------------
// Microcontroller configuration
//-------------------------------------------------------------------------------------------------
// CONFIG1L register
#pragma config CPUDIV = NOCLKDIV, CFGPLLEN = ON, PLLSEL = PLL3X // CPU uses system clock with no division, always enable the PLL, select a 3x clock multiplier factor for the PLL
// CONFIG1H register
#pragma config IESO = OFF, FCMEN = OFF, PCLKEN = ON, FOSC = INTOSCIO // Disable unneeded oscillator switchover, disable fail-safe clock monitor, always enable the primary oscillator, select the internal oscillator as the primary oscillator
// CONFIG2L register
#pragma config nLPBOR = OFF, BORV = 285, BOREN = SBORDIS, nPWRTEN = ON // Disable the low-power brown-out reset, enable the brown-out reset with the highest available voltage, enable the power up timer
// CONFIG2H register
#pragma config WDTEN = OFF // Disable the watchdog timer
// CONFIG3H register
#pragma config MCLRE = ON, SDOMX = RB3, PBADEN = OFF, CCP2MX = RC1 // Enable /MCLR, keep the SPI SDO pin on port B, configure the port B pins as digital I/Os, move the CCP2 pin to port C
// CONFIG4L register
#pragma config DEBUG = OFF, XINST = OFF, ICPRT = OFF, LVP = ON, STVREN = ON // Disable the debugger, disable the extended instruction set, disable the in-circuit debug/programming port, enable the low-voltage programming, stack overflow or underflow will reset the MCU
// CONFIG5L register
#pragma config CP3 = OFF, CP2 = OFF, CP1 = OFF, CP0 = OFF // Disable all code protections
// CONFIG5H register
#pragma config CPD = OFF, CPB = OFF // Disable data EEPROM code protection, disable boot block code protection
// CONFIG6L register
#pragma config WRT3 = ON, WRT2 = ON, WRT1 = ON, WRT0 = ON // Enable blocks write protection
// CONFIG6H register
#pragma config WRTD = OFF, WRTB = ON, WRTC = ON // Allow to write to the EEPROM, prevent writing to the boot block or the configuration registers
// CONFIG7L register
#pragma config EBTR3 = OFF, EBTR2 = OFF, EBTR1 = OFF, EBTR0 = OFF // Disable blocks read protection
// CONFIG7H register
#pragma config EBTRB = OFF // Disable boot block read protection

//-------------------------------------------------------------------------------------------------
// Private types
//-------------------------------------------------------------------------------------------------
/** The format of the CDC class specific endpoint descriptors. */
typedef struct
{
	TUSBCoreDescriptorInterface Control_Interface;
	TUSBCommunicationsFunctionalDescriptorHeader Header;
	TUSBCommunicationsFunctionalDescriptorAbstractControlManagement Management;
	TUSBCommunicationsFunctionalDescriptorUnion Union;
	TUSBCoreDescriptorEndpoint Notification_Endpoint;
	TUSBCoreDescriptorInterface Data_Interface;
	TUSBCoreDescriptorEndpoint Data_Out_Endpoint;
	TUSBCoreDescriptorEndpoint Data_In_Endpoint;
} __attribute__((packed)) TMainUSBCommunicationsClassSpecificEndpointDescriptor;

//-------------------------------------------------------------------------------------------------
// Private variables
//-------------------------------------------------------------------------------------------------
/** All application USB strings. */
static const unsigned short Main_USB_Descriptor_String_Data_0 = USB_CORE_LANGUAGE_ID_FRENCH_STANDARD;
static const unsigned short Main_USB_Descriptor_String_Data_Manufacturer[] = { 'R', 'I', 'C', 'C', 'I', 'A', 'R', 'D', 'I', ' ', 'D', 'A', 'T', 'A', ' ', 'S', 'Y', 'S', 'T', 'E', 'M' };
static const unsigned short Main_USB_Descriptor_String_Data_Product[] = { 'L', 'o', 'g', 'i', 'c', ' ', 'S', 'i', 'g', 'n', 'a', 'l', ' ', 'G', 'e', 'n', 'e', 'r', 'a', 't', 'o', 'r' };
static const unsigned short Main_USB_Descriptor_String_Data_Serial_Number[]  = { '0', '.', '1' };
static const TUSBCoreDescriptorString Main_USB_String_Descriptors[4] =
{
	{
		.bLength = USB_CORE_DESCRIPTOR_SIZE_STRING(sizeof(Main_USB_Descriptor_String_Data_0)),
		.bDescriptorType = USB_CORE_DESCRIPTOR_TYPE_STRING,
		.Pointer_Data = &Main_USB_Descriptor_String_Data_0
	},
	{
		.bLength = USB_CORE_DESCRIPTOR_SIZE_STRING(sizeof(Main_USB_Descriptor_String_Data_Manufacturer)),
		.bDescriptorType = USB_CORE_DESCRIPTOR_TYPE_STRING,
		.Pointer_Data = &Main_USB_Descriptor_String_Data_Manufacturer
	},
	{
		.bLength = USB_CORE_DESCRIPTOR_SIZE_STRING(sizeof(Main_USB_Descriptor_String_Data_Product)),
		.bDescriptorType = USB_CORE_DESCRIPTOR_TYPE_STRING,
		.Pointer_Data = &Main_USB_Descriptor_String_Data_Product
	},
	{
		.bLength = USB_CORE_DESCRIPTOR_SIZE_STRING(sizeof(Main_USB_Descriptor_String_Data_Serial_Number)),
		.bDescriptorType = USB_CORE_DESCRIPTOR_TYPE_STRING,
		.Pointer_Data = &Main_USB_Descriptor_String_Data_Serial_Number
	}
};

/** The interfaces definitions for the unique USB configuration. */
static const TMainUSBCommunicationsClassSpecificEndpointDescriptor Main_USB_Communications_Class_Specific_Endpoint_Descriptors =
{
	.Control_Interface =
	{
		.bLength = sizeof(TUSBCoreDescriptorInterface),
		.bDescriptorType = USB_CORE_DESCRIPTOR_TYPE_INTERFACE,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 1,
		.bInterfaceClass = USB_CORE_INTERFACE_CLASS_CODE_COMMUNICATIONS,
		.bInterfaceSubClass = USB_CORE_INTERFACE_SUB_CLASS_CODE_ABSTRACT_CONTROL_MODEL,
		.bInterfaceProtocol = USB_CORE_INTERFACE_PROTOCOL_CODE_ITU_V250,
		.iInterface = 0
	},
	.Header =
	{
		.bFunctionLength = sizeof(TUSBCommunicationsFunctionalDescriptorHeader),
		.bDescriptorType = USB_COMMUNICATIONS_FUNCTIONAL_DESCRIPTOR_TYPE_INTERFACE,
		.bDescriptorSubtype = USB_COMMUNICATIONS_FUNCTIONAL_DESCRIPTOR_SUB_TYPE_HEADER,
		.bcdCDC = USB_COMMUNICATIONS_SPECIFICATION_RELEASE_NUMBER
	},
	.Management =
	{
		.bFunctionLength = sizeof(TUSBCommunicationsFunctionalDescriptorAbstractControlManagement),
		.bDescriptorType = USB_COMMUNICATIONS_FUNCTIONAL_DESCRIPTOR_TYPE_INTERFACE,
		.bDescriptorSubtype = USB_COMMUNICATIONS_FUNCTIONAL_DESCRIPTOR_SUB_TYPE_ABSTRACT_CONTROL_MANAGEMENT,
		.bmCapabilities = 0 // TODO configure the capabilities
	},
	.Union =
	{
		.bFunctionLength = sizeof(TUSBCommunicationsFunctionalDescriptorUnion),
		.bDescriptorType = USB_COMMUNICATIONS_FUNCTIONAL_DESCRIPTOR_TYPE_INTERFACE,
		.bDescriptorSubtype = USB_COMMUNICATIONS_FUNCTIONAL_DESCRIPTOR_SUB_TYPE_UNION,
		.bControlInterface = 0,
		.bSubordinateInterface0 = 1
	},
	.Notification_Endpoint =
	{
		.bLength = sizeof(TUSBCoreDescriptorEndpoint),
		.bDescriptorType = USB_CORE_DESCRIPTOR_TYPE_ENDPOINT,
		.bEndpointAddress = 1 | USB_CORE_DESCRIPTOR_ENDPOINT_ATTRIBUTE_ENDPOINT_ADDRESS_DIRECTION_IN,
		.bmAttributes = USB_CORE_DESCRIPTOR_ENDPOINT_ATTRIBUTE_TRANSFER_TYPE_INTERRUPT,
		.wMaxPacketSize = 8,
		.bInterval = 255
	},
	.Data_Interface =
	{
		.bLength = sizeof(TUSBCoreDescriptorInterface),
		.bDescriptorType = USB_CORE_DESCRIPTOR_TYPE_INTERFACE,
		.bInterfaceNumber = 1,
		.bAlternateSetting = 0,
		.bNumEndpoints = 2,
		.bInterfaceClass = USB_CORE_INTERFACE_CLASS_CODE_DATA_INTERFACE,
		.bInterfaceSubClass = USB_CORE_INTERFACE_SUB_CLASS_CODE_NONE,
		.bInterfaceProtocol = USB_CORE_INTERFACE_PROTOCOL_CODE_NONE,
		.iInterface = 0
	},
	.Data_Out_Endpoint =
	{
		.bLength = sizeof(TUSBCoreDescriptorEndpoint),
		.bDescriptorType = USB_CORE_DESCRIPTOR_TYPE_ENDPOINT,
		.bEndpointAddress = 2 | USB_CORE_DESCRIPTOR_ENDPOINT_ATTRIBUTE_ENDPOINT_ADDRESS_DIRECTION_OUT,
		.bmAttributes = USB_CORE_DESCRIPTOR_ENDPOINT_ATTRIBUTE_TRANSFER_TYPE_BULK,
		.wMaxPacketSize = USB_CORE_ENDPOINT_PACKETS_SIZE,
		.bInterval = 1
	},
	.Data_In_Endpoint =
	{
		.bLength = sizeof(TUSBCoreDescriptorEndpoint),
		.bDescriptorType = USB_CORE_DESCRIPTOR_TYPE_ENDPOINT,
		.bEndpointAddress = 3 | USB_CORE_DESCRIPTOR_ENDPOINT_ATTRIBUTE_ENDPOINT_ADDRESS_DIRECTION_IN,
		.bmAttributes = USB_CORE_DESCRIPTOR_ENDPOINT_ATTRIBUTE_TRANSFER_TYPE_BULK,
		.wMaxPacketSize = USB_CORE_ENDPOINT_PACKETS_SIZE,
		.bInterval = 1
	}
};

/** The application unique USB configuration descriptor. */
static const TUSBCoreDescriptorConfiguration Main_USB_Configuration_Descriptor = // Store this into the program memory to save some RAM
{
	.bLength = USB_CORE_DESCRIPTOR_SIZE_CONFIGURATION,
	.bDescriptorType = USB_CORE_DESCRIPTOR_TYPE_CONFIGURATION,
	.wTotalLength = USB_CORE_DESCRIPTOR_SIZE_CONFIGURATION + sizeof(Main_USB_Communications_Class_Specific_Endpoint_Descriptors),
	.bNumInterfaces = 2,
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0, // The device is not self-powered and does not support the remove wakeup feature
	.bMaxPower = 250, // Take as much power as possible, just in case the logic signal generator needs to power a board
	.Pointer_Interfaces_Data = &Main_USB_Communications_Class_Specific_Endpoint_Descriptors
};

// TEST
static void MainSimpleEcho(TUSBCoreHardwareEndpointOutTransferCallbackData *Pointer_Transfer_Callback_Data)
{
	static unsigned char Sync = 1;

	LOG(1, "reception");

	// Manage the USB connection
	USBCorePrepareForOutTransfer(Pointer_Transfer_Callback_Data->Endpoint_ID, Sync); // Re-enable packets reception
	if (Sync == 0) Sync = 1;
	else Sync = 0;

	USBCorePrepareForInTransfer(3, Pointer_Transfer_Callback_Data->Pointer_OUT_Data_Buffer, Pointer_Transfer_Callback_Data->Data_Size, Sync ? 0 : 1);
}

/** Each used hardware USB endpoint configuration. */
static TUSBCoreHardwareEndpointConfiguration Main_USB_Hardware_Endpoints_Configuration[] =
{
	// Control endpoint
	{
		.Enabled_Directions = USB_CORE_HARDWARE_ENDPOINT_DIRECTION_OUT | USB_CORE_HARDWARE_ENDPOINT_DIRECTION_IN,
		.Out_Transfer_Callback = USBCommunicationsHandleControlRequestCallback,
		.In_Transfer_Callback = NULL
	},
	// CDC ACM notification
	{
		.Enabled_Directions = USB_CORE_HARDWARE_ENDPOINT_DIRECTION_IN,
		.Out_Transfer_Callback = NULL,
		.In_Transfer_Callback = NULL
	},
	// CDC ACM data OUT
	{
		.Enabled_Directions = USB_CORE_HARDWARE_ENDPOINT_DIRECTION_OUT,
		.Out_Transfer_Callback = MainSimpleEcho, // Only this endpoint can receive data
		.In_Transfer_Callback = NULL
	},
	// CDC ACM data IN
	{
		.Enabled_Directions = USB_CORE_HARDWARE_ENDPOINT_DIRECTION_OUT | USB_CORE_HARDWARE_ENDPOINT_DIRECTION_IN,
		.Out_Transfer_Callback = NULL,
		.In_Transfer_Callback = USBCommunicationsHandleTransmissionFlowControlCallback
	}
};

/** The application USB device descriptor, see chapter 5.1.1 of the Class Definitions for Communications Devices revision 1.2. */
static const TUSBCoreDescriptorDevice Main_USB_Device_Descriptor = // Store this into the program memory to save some RAM
{
	.bLength = USB_CORE_DESCRIPTOR_SIZE_DEVICE,
	.bDescriptorType = USB_CORE_DESCRIPTOR_TYPE_DEVICE,
	.bcdUSB = USB_CORE_BCD_USB_SPECIFICATION_RELEASE_NUMBER,
	.bDeviceClass = USB_CORE_DEVICE_CLASS_CODE_COMMUNICATIONS,
	.bDeviceSubClass = USB_CORE_DEVICE_SUB_CLASS_CODE_NONE, // Tell the host to check each interface
	.bDeviceProtocol = USB_CORE_DEVICE_PROTOCOL_CODE_NONE, // Unused
	.bMaxPacketSize0 = USB_CORE_ENDPOINT_PACKETS_SIZE,
	.idVendor = 0x1240, // Use the Microchip VID for now
	.idProduct = 0xFADA, // Use a random product ID
	.bcdDevice = 0x0001, // Version 0.1 for now
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
	.Pointer_Configurations = &Main_USB_Configuration_Descriptor,
	.Pointer_Strings = Main_USB_String_Descriptors,
	.String_Descriptors_Count = USB_CORE_ARRAY_SIZE(Main_USB_String_Descriptors),
	.Pointer_Hardware_Endpoints_Configuration = Main_USB_Hardware_Endpoints_Configuration,
	.Hardware_Endpoints_Count = USB_CORE_ARRAY_SIZE(Main_USB_Hardware_Endpoints_Configuration)
};

//-------------------------------------------------------------------------------------------------
// Private functions
//-------------------------------------------------------------------------------------------------
/** High-priority interrupts handler entry point. */
void __interrupt(high_priority) MainInterruptHandlerHighPriority(void)
{
	if (USB_CORE_IS_INTERRUPT_FIRED()) USBCoreInterruptHandler();
}

/** Low-priority interrupts handler entry point. */
void __interrupt(low_priority) MainInterruptHandlerLowPriority(void)
{
}

//-------------------------------------------------------------------------------------------------
// Entry point
//-------------------------------------------------------------------------------------------------
void main(void)
{
	// Configure the system clock at 48MHz
	OSCCON = 0x70; // Select a 16MHz frequency output for the internal oscillator, select the primary clock configured by the fuses (which is the internal oscillator)
	__delay_ms(10); // Add a little delay to make sure that the PLL is locked (2ms should be enough, but take some margin)

	// Initialize the modules
	UARTInitialize();

	// Configure the interrupts
	RCONbits.IPEN = 1; // Enable priority levels on interrupts
	INTCON |= 0xC0; // Enable high and low priority interrupts

	LOG(1, "\033[33mInitialization complete.\033[0m");

	// Initialize the USB stack now that all modules are operational
	USBCoreInitialize(&Main_USB_Device_Descriptor);
	USBCommunicationsInitialize(3);

	// TEST
	for (int i = 0; i < 10; i++) __delay_ms(1000);
	USBCommunicationsWriteString("Ceci est un test de chaîne\r\n");
	USBCommunicationsWriteString("Voici la seconde chaîne\r\n");
	USBCommunicationsWriteString("Et ceci est la troisième et dernière chaîne\r\n");

	// TEST
	ANSELBbits.ANSB2 = 0;
	LATBbits.LATB2 = 0;
	TRISBbits.TRISB2 = 0;
	while (1)
	{
		UARTWriteByte('C');
		UARTWriteByte('I');
		UARTWriteByte('A');
		UARTWriteByte('O');
		UARTWriteByte('\r');
		UARTWriteByte('\n');

		LATBbits.LATB2 = !LATBbits.LATB2;
		__delay_ms(1000);
	}
}
