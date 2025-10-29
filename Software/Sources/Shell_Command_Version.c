/** @file Shell_Command_Version.c
 * Implement the shell "version" command.
 * @author Adrien RICCIARDI
 */
#include <Shell_Commands.h>
#include <USB_Communications.h>

//-------------------------------------------------------------------------------------------------
// Private constants
//-------------------------------------------------------------------------------------------------
/** The firmware version. */
#define SHELL_COMMAND_VERSION_STRING "1.0"

//-------------------------------------------------------------------------------------------------
// Public functions
//-------------------------------------------------------------------------------------------------
void ShellCommandVersionCallback(char __attribute__((unused)) *Pointer_String_Arguments)
{
	USBCommunicationsWriteString("\r\nFirmware version : " SHELL_COMMAND_VERSION_STRING "\r\nBuild date : " __DATE__ "\r\nBuild time : " __TIME__);
}
