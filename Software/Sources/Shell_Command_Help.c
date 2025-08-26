/** @file Shell_Command_Help.c
 * Implement the shell "help" command.
 * @author Adrien RICCIARDI
 */
#include <Shell_Commands.h>
#include <USB_Communications.h>

//-------------------------------------------------------------------------------------------------
// Public functions
//-------------------------------------------------------------------------------------------------
void ShellCommandHelpCallback(char __attribute__((unused)) *Pointer_String_Arguments)
{
	unsigned char i;
	const TShellCommand *Pointer_Command = Shell_Commands;

	// Display all available commands
	USBCommunicationsWriteString("\r\nAvailable commands :");
	for (i = 0; i < SHELL_COMMANDS_COUNT; i++)
	{
		// Send a lot of USB packets, which takes a bit of time, but this avoids using one more buffer
		USBCommunicationsWriteString("\r\n  ");
		USBCommunicationsWriteString((char *) Pointer_Command->Pointer_String_Command);
		USBCommunicationsWriteString(" : ");
		USBCommunicationsWriteString((char *) Pointer_Command->Pointer_String_Description);

		Pointer_Command++;
	}
}
