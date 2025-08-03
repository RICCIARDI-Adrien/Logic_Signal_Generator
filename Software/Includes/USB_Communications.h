/** @file USB_Communications.h
 * The USB Communications class.
 * @author Adrien RICCIARDI
 */
#ifndef H_USB_COMMUNICATIONS_H
#define H_USB_COMMUNICATIONS_H

#include <USB_Core.h>

//-------------------------------------------------------------------------------------------------
// Constants
//-------------------------------------------------------------------------------------------------
/** The Class Definitions for Communications Devices document revision 1.2 release number in little-endian BCD format. */
#define USB_COMMUNICATIONS_SPECIFICATION_RELEASE_NUMBER { 0x02, 0x01 }

//-------------------------------------------------------------------------------------------------
// Types
//-------------------------------------------------------------------------------------------------
/** All supported descriptor types. */
typedef enum : unsigned char
{
	USB_COMMUNICATIONS_FUNCTIONAL_DESCRIPTOR_TYPE_INTERFACE = 0x24,
	USB_COMMUNICATIONS_FUNCTIONAL_DESCRIPTOR_TYPE_ENDPOINT = 0x25
} TUSBCommunicationsFunctionalDescriptorType;

/** All supported descriptor sub types. */
typedef enum : unsigned char
{
	USB_COMMUNICATIONS_FUNCTIONAL_DESCRIPTOR_SUB_TYPE_HEADER = 0,
	USB_COMMUNICATIONS_FUNCTIONAL_DESCRIPTOR_SUB_TYPE_ABSTRACT_CONTROL_MANAGEMENT = 2,
	USB_COMMUNICATIONS_FUNCTIONAL_DESCRIPTOR_SUB_TYPE_UNION = 6
} TUSBCommunicationsFunctionalDescriptorSubType;

/** An USB communication Header functional descriptor using the USB naming for simplicity. See the USB CDC specifications 1.2 table 15. */
typedef struct
{
	unsigned char bFunctionLength;
	TUSBCommunicationsFunctionalDescriptorType bDescriptorType;
	TUSBCommunicationsFunctionalDescriptorSubType bDescriptorSubtype;
	unsigned char bcdCDC[2];
} __attribute__((packed)) TUSBCommunicationsFunctionalDescriptorHeader;

/** An USB communication Abstract Control Management functional descriptor using the USB naming for simplicity. See the USB CDC PSTN specifications 1.2 table 4. */
typedef struct
{
	unsigned char bFunctionLength;
	TUSBCommunicationsFunctionalDescriptorType bDescriptorType;
	TUSBCommunicationsFunctionalDescriptorSubType bDescriptorSubtype;
	unsigned char bmCapabilities;
} __attribute__((packed)) TUSBCommunicationsFunctionalDescriptorAbstractControlManagement;

/** An USB communication Union functional descriptor using the USB naming for simplicity. See the USB CDC PSTN specifications 1.2 table 16.
 * @note Only one subordinate interface is currently supported.
 */
typedef struct
{
	unsigned char bFunctionLength;
	TUSBCommunicationsFunctionalDescriptorType bDescriptorType;
	TUSBCommunicationsFunctionalDescriptorSubType bDescriptorSubtype;
	unsigned char bControlInterface;
	unsigned char bSubordinateInterface0;
} __attribute__((packed)) TUSBCommunicationsFunctionalDescriptorUnion;

/** All supported PSTN class-specific request codes. See CDC PSTN revision 1.2 table 13. */
typedef enum : unsigned char
{
	USB_COMMUNICATIONS_PSTN_REQUEST_CODE_SET_LINE_CODING = 0x20,
	USB_COMMUNICATIONS_PSTN_REQUEST_CODE_SET_CONTROL_LINE_STATE = 0x22
} TUSBCommunicationsPSTNRequestCode;

/** The PSTN Get/Set Line Coding request payload. */
typedef struct
{
	unsigned long dwDTERate;
	unsigned char bCharFormat;
	unsigned char bParityType;
	unsigned char bDataBits;
} __attribute__((packed)) TUSBCommunicationsPSTNRequestGetLineCodingPayload;

//-------------------------------------------------------------------------------------------------
// Functions
//-------------------------------------------------------------------------------------------------
/** Process a CDC ACM control request.
 * @param Pointer_Transfer_Callback_Data The request data.
 */
void USBCommunicationsHandleControlRequest(TUSBCoreHardwareEndpointTransferCallbackData *Pointer_Transfer_Callback_Data);

#endif
