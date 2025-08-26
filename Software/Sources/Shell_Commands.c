/** @file Shell_Commands.c
 * See Shell_Commands.h for description.
 * @author Adrien RICCIARDI
 */
#include <Shell_Commands.h>

//-------------------------------------------------------------------------------------------------
// Public variables
//-------------------------------------------------------------------------------------------------
const TShellCommand Shell_Commands[SHELL_COMMANDS_COUNT] =
{
	// Help
	{
		.Pointer_String_Command = "help",
		.Pointer_String_Description = "Show this commands list.",
		.Command_Callback = ShellCommandHelpCallback
	}
};
