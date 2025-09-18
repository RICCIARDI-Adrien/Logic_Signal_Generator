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
	// I2C
	{
		.Pointer_String_Command = "i2c",
		.Pointer_String_Description = "send an I2C transaction on the bus. Use \"[\" for start, \"]\" for stop, \"r[h]XXXX\" for reading XXXX bytes, then \"XX\" or \"hXX\" to write a decimal or a hexadecimal byte.",
		.Command_Callback = ShellCommandI2CCallback
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
	},
	// Pinout
	{
		.Pointer_String_Command = "pinout",
		.Pointer_String_Description = "show the pins wiring corresponding to each supported protocol.",
		.Command_Callback = ShellCommandPinoutCallback
	},
	// SPI configure
	{
		.Pointer_String_Command = "spi-configure",
		.Pointer_String_Description = "set the SPI interface settings. Usage : \"spi-configure 50khz|100khz|500khz|1mhz|2mhz mode0|mode1|mode2|mode3\".",
		.Command_Callback = ShellCommandSPIConfigureCallback
	}
};
