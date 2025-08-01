/** @file USB_Communications.c
 * See USB_Communications.h.
 * @author Adrien RICCIARDI
 */
#include <Log.h>
#include <USB_Communications.h>

//-------------------------------------------------------------------------------------------------
// Private constants
//-------------------------------------------------------------------------------------------------
/** Set to 1 to enable the log messages, set to 0 to disable them. */
#define USB_COMMUNICATIONS_IS_LOGGING_ENABLED 1

//-------------------------------------------------------------------------------------------------
// Public functions
//-------------------------------------------------------------------------------------------------
void USBCommunicationsHandleControlRequest(TUSBCoreHardwareEndpointTransferCallbackData *Pointer_Transfer_Callback_Data)
{
	USBCorePrepareForInTransfer(Pointer_Transfer_Callback_Data->Endpoint_ID, NULL, 0, 1); // Send back an empty packet to acknowledge the command reception

	// Re-enable packets reception
	USBCorePrepareForOutTransfer(Pointer_Transfer_Callback_Data->Endpoint_ID, 1);
}
