/** @file Shell.c
 * See Shell.h for description.
 * @author Adrien RICCIARDI
 */
#include <Log.h>
#include <Shell.h>
#include <USB_Communications.h>

//-------------------------------------------------------------------------------------------------
// Private constants
//-------------------------------------------------------------------------------------------------
/** Set to 1 to enable the log messages, set to 0 to disable them. */
#define SHELL_IS_LOGGING_ENABLED 1

//-------------------------------------------------------------------------------------------------
// Public functions
//-------------------------------------------------------------------------------------------------
void ShellReadCommandLine(char *Pointer_String_Command_Line, unsigned char Maximum_Length)
{
	char Character;
	unsigned char Length = 0;

	// Immediately return with an empty string if the provided maximum length is too small
	if (Maximum_Length <= 1)
	{
		if (Maximum_Length > 0) goto End;
		return;
	}
	// Decrement the maximum length by one to always keep room for the terminating zero and to avoid computing "Maximum_Length - 1" in every loop of the following "while"
	Maximum_Length--;

	// Display the prompt
	USBCommunicationsWriteString("\r\n> ");

	while (1)
	{
		Character = USBCommunicationsReadCharacter();
		switch (Character)
		{
			case '\b':
			case 127: // VT100 terminals send the DEL character when pressing backspace
				// Remove the last character only if there is one
				if (Length > 0)
				{
					// Go back one character, erase it then go back again
					USBCommunicationsWriteString("\b \b");
					Pointer_String_Command_Line--;
					Length--;
				}
				break;

			// Terminate the string
			case '\n':
			case '\r':
				goto End;

			// Append the character to the string
			default:
				// Discard the character if the maximum command line size is reached
				if (Length < Maximum_Length)
				{
					*Pointer_String_Command_Line = Character;
					Pointer_String_Command_Line++;
					Length++;
					USBCommunicationsWriteCharacter(Character);
				}
				break;
		}
	}

End:
	// Terminate the string
	*Pointer_String_Command_Line = 0;
}
