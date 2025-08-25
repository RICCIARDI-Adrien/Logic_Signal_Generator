/** @file USB_Communications.c
 * See USB_Communications.h.
 * @author Adrien RICCIARDI
 */
#include <Log.h>
#include <string.h>
#include <USB_Communications.h>

//-------------------------------------------------------------------------------------------------
// Private constants
//-------------------------------------------------------------------------------------------------
/** Set to 1 to enable the log messages, set to 0 to disable them. */
#define USB_COMMUNICATIONS_IS_LOGGING_ENABLED 1

/** The size in bytes of the reception circular buffer. */
#define USB_COMMUNICATIONS_DATA_RECEPTION_BUFFER_SIZE 8 // TODO start with a small buffer to assert the performances

//-------------------------------------------------------------------------------------------------
// Private types
//-------------------------------------------------------------------------------------------------
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
// Private variables
//-------------------------------------------------------------------------------------------------
/** Keep the data synchronization value for the data OUT endpoint communication. */
static unsigned char USB_Communications_Data_Out_Endpoint_Data_Synchronization = 1; // The first packet sent by the host has the synchronization value 0, so expect a 1 for the next packet

/** Cache the number corresponding to the data IN endpoint. */
static unsigned char USB_Communications_Data_In_Endpoint_ID;
/** Keep the data synchronization value for the data IN endpoint communication. */
static unsigned char USB_Communications_Data_In_Endpoint_Data_Synchronization = 0;

/** Store the last received data bytes. */
static unsigned char USB_Communications_Data_Reception_Buffer[USB_COMMUNICATIONS_DATA_RECEPTION_BUFFER_SIZE];
/** The beginning of the received data that are not yet read by the user. */
static unsigned char *Pointer_USB_Communications_Data_Reception_Buffer_Reading = USB_Communications_Data_Reception_Buffer;
/** The beginning of the buffer free area to write incoming data to. */
static unsigned char *Pointer_USB_Communications_Data_Reception_Buffer_Writing = USB_Communications_Data_Reception_Buffer;
/** The occupancy of the buffer. */
static volatile unsigned char USB_Communications_Data_Reception_Buffer_Occupied_Bytes_Count = 0;

/** A synchronization flag telling whether the data transmission path is ready. */
static volatile unsigned char USB_Communications_Is_Transmission_Finished = 1; // No transmission has taken place yet

//-------------------------------------------------------------------------------------------------
// Public functions
//-------------------------------------------------------------------------------------------------
void USBCommunicationsHandleControlRequestCallback(TUSBCoreHardwareEndpointOutTransferCallbackData *Pointer_Transfer_Callback_Data)
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
		LOG(USB_COMMUNICATIONS_IS_LOGGING_ENABLED, "Received a request with the code 0x%02X.", Last_Request_Code);

		// Prepare the state machine for the payload reception
		if (Pointer_Request->wLength > 0)
		{
			LOG(USB_COMMUNICATIONS_IS_LOGGING_ENABLED, "Expecting a payload of %u bytes.", Pointer_Request->wLength);
			Current_State = STATE_RECEIVE_PAYLOAD;

			// The request (sent by the host) data synchronization is always 0, so wait for a 1 for the next packet containing the payload
			// Do not acknowledge the packet reception with an empty IN packet because we are waiting for the payload OUT one
			USBCorePrepareForOutTransfer(Pointer_Transfer_Callback_Data->Endpoint_ID, 1);
			return;
		}
		else
		{
			LOG(USB_COMMUNICATIONS_IS_LOGGING_ENABLED, "No payload is expected, processing the request.");

			// Process the request
			switch (Last_Request_Code)
			{
				case USB_COMMUNICATIONS_PSTN_REQUEST_CODE_SET_CONTROL_LINE_STATE:
					// This request is ignored
					LOG(USB_COMMUNICATIONS_IS_LOGGING_ENABLED, "Processing the Set Control Line State PSTN request.");
					break;

				default:
					LOG(USB_COMMUNICATIONS_IS_LOGGING_ENABLED, "Unsupported request 0x%02X.", Last_Request_Code);
					break;
			}
		}
	}
	// The payload has been received
	else
	{
		LOG(USB_COMMUNICATIONS_IS_LOGGING_ENABLED, "The request payload of %u bytes has been received.", Pointer_Transfer_Callback_Data->Data_Size);

		// Process the request
		switch (Last_Request_Code)
		{
			case USB_COMMUNICATIONS_PSTN_REQUEST_CODE_SET_LINE_CODING:
				// This request is ignored, only display the payload content
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
				LOG(USB_COMMUNICATIONS_IS_LOGGING_ENABLED, "Unsupported request 0x%02X.", Last_Request_Code);
				break;
		}

		// Wait for the next request
		Current_State = STATE_RECEIVE_REQUEST;
	}

	// Manage the USB connection
	USBCorePrepareForInTransfer(Pointer_Transfer_Callback_Data->Endpoint_ID, NULL, 0, 1); // Send back an empty packet to acknowledge the command reception
	USBCorePrepareForOutTransfer(Pointer_Transfer_Callback_Data->Endpoint_ID, 0); // Re-enable packets reception
}

void USBCommunicationsHandleDataReceptionCallback(TUSBCoreHardwareEndpointOutTransferCallbackData *Pointer_Transfer_Callback_Data)
{
	// Cache the callback data parameters to avoid useless pointer computations in the loop
	unsigned char Received_Bytes_Count = Pointer_Transfer_Callback_Data->Data_Size, *Pointer_Received_Data_Buffer = Pointer_Transfer_Callback_Data->Pointer_OUT_Data_Buffer;

	LOG(USB_COMMUNICATIONS_IS_LOGGING_ENABLED, "Received %u bytes of data.", Received_Bytes_Count);

	// Atomic access to the shared FIFO is granted by the fact that the user-callable function temporarily disables the USB interrupts, so it is not possible to reach this code at the critical moment
	// Append the received data if there is still room in the buffer
	if (USB_Communications_Data_Reception_Buffer_Occupied_Bytes_Count < USB_COMMUNICATIONS_DATA_RECEPTION_BUFFER_SIZE)
	{
		// Append the received data until the reception buffer is full or all data have been appended
		while ((Received_Bytes_Count > 0) && (USB_Communications_Data_Reception_Buffer_Occupied_Bytes_Count < USB_COMMUNICATIONS_DATA_RECEPTION_BUFFER_SIZE))
		{
			// Wrap around the pointer to the beginning of the buffer when it has reached the end of the buffer in a minimum amount of cycles
			if (Pointer_USB_Communications_Data_Reception_Buffer_Writing == (USB_Communications_Data_Reception_Buffer + USB_COMMUNICATIONS_DATA_RECEPTION_BUFFER_SIZE)) Pointer_USB_Communications_Data_Reception_Buffer_Writing = USB_Communications_Data_Reception_Buffer;

			// Store the next received byte into the buffer
			*Pointer_USB_Communications_Data_Reception_Buffer_Writing = *Pointer_Received_Data_Buffer;
			Pointer_USB_Communications_Data_Reception_Buffer_Writing++;
			Pointer_Received_Data_Buffer++;
			USB_Communications_Data_Reception_Buffer_Occupied_Bytes_Count++;
			Received_Bytes_Count--;
		}
	}
	else LOG(USB_COMMUNICATIONS_IS_LOGGING_ENABLED, "Warning : the reception buffer is full, all the received data have been discarded.");

	// Re-enable packets reception
	USBCorePrepareForOutTransfer(Pointer_Transfer_Callback_Data->Endpoint_ID, USB_Communications_Data_Out_Endpoint_Data_Synchronization);

	// Update the synchronization value
	if (USB_Communications_Data_Out_Endpoint_Data_Synchronization == 0) USB_Communications_Data_Out_Endpoint_Data_Synchronization = 1;
	else USB_Communications_Data_Out_Endpoint_Data_Synchronization = 0;
}

void USBCommunicationsHandleDataTransmissionFlowControlCallback(unsigned char __attribute__((unused)) Endpoint_ID)
{
	USB_Communications_Is_Transmission_Finished = 1;
}

void USBCommunicationsInitialize(unsigned char Data_In_Endpoint_ID)
{
	USB_Communications_Data_In_Endpoint_ID = Data_In_Endpoint_ID;
}

char USBCommunicationsReadCharacter(void)
{
	unsigned char Character;

	// Wait for a character to be received
	// Accessing the occupied bytes count single-byte variable without the atomic access protections is safe because this is just a read, doing this avoids disabling the USB interrupts for too long
	while (USB_Communications_Data_Reception_Buffer_Occupied_Bytes_Count == 0);

	// Wrap around the pointer to the beginning of the buffer when it has reached the end of the buffer in a minimum amount of cycles, this can alse be done without atomic access protections because only this function is accessing this pointer value
	if (Pointer_USB_Communications_Data_Reception_Buffer_Reading == (USB_Communications_Data_Reception_Buffer + USB_COMMUNICATIONS_DATA_RECEPTION_BUFFER_SIZE)) Pointer_USB_Communications_Data_Reception_Buffer_Reading = USB_Communications_Data_Reception_Buffer;

	// Atomically access to the reception circular buffer
	USB_CORE_INTERRUPT_DISABLE();
	Character = *Pointer_USB_Communications_Data_Reception_Buffer_Reading;
	Pointer_USB_Communications_Data_Reception_Buffer_Reading++;
	USB_Communications_Data_Reception_Buffer_Occupied_Bytes_Count--;
	USB_CORE_INTERRUPT_ENABLE();

	return (char) Character;
}

void USBCommunicationsWriteCharacter(char Character)
{
	LOG(USB_COMMUNICATIONS_IS_LOGGING_ENABLED, "Writing the character '%c'.", Character);

	// Wait for the previous transmission to end
	while (!USB_Communications_Is_Transmission_Finished);
	USB_Communications_Is_Transmission_Finished = 0;

	// Provide the next chunk of data to transmit
	USBCorePrepareForInTransfer(USB_Communications_Data_In_Endpoint_ID, &Character, 1, USB_Communications_Data_In_Endpoint_Data_Synchronization);

	// Update the synchronization value
	if (USB_Communications_Data_In_Endpoint_Data_Synchronization == 0) USB_Communications_Data_In_Endpoint_Data_Synchronization = 1;
	else USB_Communications_Data_In_Endpoint_Data_Synchronization = 0;
}

void USBCommunicationsWriteString(char *Pointer_String)
{
	size_t Length;
	unsigned char Chunk_Length;

	// Cache the string length
	Length = strlen(Pointer_String);
	LOG(USB_COMMUNICATIONS_IS_LOGGING_ENABLED, "Writing the string \"%s\" made of %u bytes.", Pointer_String, Length);

	// Send the string in chunks if its size exceeds the USB packet size
	while (Length > 0)
	{
		// Wait for the previous transmission to end
		while (!USB_Communications_Is_Transmission_Finished);
		USB_Communications_Is_Transmission_Finished = 0;

		// Split the data in chunks if needed
		if (Length > USB_CORE_ENDPOINT_PACKETS_SIZE) Chunk_Length = USB_CORE_ENDPOINT_PACKETS_SIZE;
		else Chunk_Length = (unsigned char) Length;
		LOG(USB_COMMUNICATIONS_IS_LOGGING_ENABLED, "Sending a data chunk of %u bytes.", Chunk_Length);

		// Provide the next chunk of data to transmit
		USBCorePrepareForInTransfer(USB_Communications_Data_In_Endpoint_ID, Pointer_String, Chunk_Length, USB_Communications_Data_In_Endpoint_Data_Synchronization);

		// Update the synchronization value
		if (USB_Communications_Data_In_Endpoint_Data_Synchronization == 0) USB_Communications_Data_In_Endpoint_Data_Synchronization = 1;
		else USB_Communications_Data_In_Endpoint_Data_Synchronization = 0;

		// Prepare for the next chunk transmission
		Pointer_String += Chunk_Length;
		Length -= Chunk_Length;
	}
}
