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
	typedef enum
	{
		STATE_RECEIVE_REQUEST,
		STATE_RECEIVE_PAYLOAD
	} TState;
	static TState Current_State = STATE_RECEIVE_REQUEST;
	static TUSBCommunicationsPSTNRequestCode Last_Request_Code;

	LOG(USB_COMMUNICATIONS_IS_LOGGING_ENABLED, "Entry, current state : %u.", Current_State);

	// A simple state machine to deal with the request packet, that may be followed by a payload packet
	if (Current_State == STATE_RECEIVE_REQUEST)
	{
		// Check wether a payload is expected
		TUSBCoreDeviceRequest *Pointer_Request = (TUSBCoreDeviceRequest *) Pointer_Transfer_Callback_Data->Pointer_OUT_Data_Buffer;
		Last_Request_Code = (TUSBCommunicationsPSTNRequestCode) Pointer_Request->bRequest;
		LOG(USB_COMMUNICATIONS_IS_LOGGING_ENABLED, "Received a request with the code %u.", Last_Request_Code);

		// Prepare the state machine for the payload reception
		if (Pointer_Request->wLength > 0)
		{
			LOG(USB_COMMUNICATIONS_IS_LOGGING_ENABLED, "Expecting a payload of %u bytes.", Pointer_Request->wLength);
			Current_State = STATE_RECEIVE_PAYLOAD;
		}
		else
		{
			LOG(USB_COMMUNICATIONS_IS_LOGGING_ENABLED, "No payload is expected, processing the request.");
			// TODO no request without payload is supported yet
		}
	}
	// The payload has been received
	else
	{
		// Process the request
		switch (Last_Request_Code)
		{
			case USB_COMMUNICATIONS_PSTN_REQUEST_CODE_SET_LINE_CODING:
				// This command is ignored, only display the payload content
				LOG_BEGIN_SECTION(USB_COMMUNICATIONS_IS_LOGGING_ENABLED)
				{
					TUSBCommunicationsPSTNRequestGetLineCodingPayload *Pointer_Payload = (TUSBCommunicationsPSTNRequestGetLineCodingPayload *) Pointer_Transfer_Callback_Data->Pointer_OUT_Data_Buffer;

					// Convert the parity to a string
					const char *Pointer_String_Parity;
					switch (Pointer_Payload->bParityType)
					{
						case 0:
							Pointer_String_Parity = "none";
							break;
						case 1:
							Pointer_String_Parity = "odd";
							break;
						case 2:
							Pointer_String_Parity = "even";
							break;
						case 3:
							Pointer_String_Parity = "mark";
							break;
						case 4:
							Pointer_String_Parity = "space";
							break;
						default:
							Pointer_String_Parity = "unknown (error)";
							break;
					}
					// TODO  Even if the code section is discarded in release mode, there is still a warning about the Pointer_String_Parity variable which is unused
					(void) Pointer_String_Parity;

					LOG(USB_COMMUNICATIONS_IS_LOGGING_ENABLED, "Processing the Set Line Coding PSTN request. Baud rate : %lu bits/s, stop bits : %u, parity : %s, data bits : %u.", Pointer_Payload->dwDTERate, Pointer_Payload->bCharFormat, Pointer_String_Parity, Pointer_Payload->bDataBits);
				}
				LOG_END_SECTION()

				break;

			default:
				LOG(USB_COMMUNICATIONS_IS_LOGGING_ENABLED, "Unsupported request %u.", Last_Request_Code);
				break;
		}
	}

	// Manage the USB connection
	USBCorePrepareForInTransfer(Pointer_Transfer_Callback_Data->Endpoint_ID, NULL, 0, 1); // Send back an empty packet to acknowledge the command reception
	USBCorePrepareForOutTransfer(Pointer_Transfer_Callback_Data->Endpoint_ID, 1); // Re-enable packets reception
}
