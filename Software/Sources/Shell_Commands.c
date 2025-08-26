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
		.Pointer_String_Description = "show this commands list.",
		.Command_Callback = ShellCommandHelpCallback
	},
	// I2C configure
	{
		.Pointer_String_Command = "i2c-configure",
		.Pointer_String_Description = "set the I2C interface settings. Usage : \"i2c-configure 100khz|400khz\".",
		.Command_Callback = ShellCommandI2CConfigureCallback
	},
	// I2C scan
	{
		.Pointer_String_Command = "i2c-scan",
		.Pointer_String_Description = "scan the I2C bus from address 1 to 127.",
		.Command_Callback = ShellCommandI2CScanCallback
	}
};
