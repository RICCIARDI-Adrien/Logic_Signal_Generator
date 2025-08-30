/** @file Shell_Command_I2C.c
 * Implement all I2C-related shell commands.
 * @author Adrien RICCIARDI
 */
#include <Log.h>
#include <MSSP.h>
#include <Shell.h>
#include <Shell_Commands.h>
#include <stdio.h>
#include <USB_Communications.h>
#include <Utility.h>

//-------------------------------------------------------------------------------------------------
// Private constants
//-------------------------------------------------------------------------------------------------
/** Set to 1 to enable the log messages, set to 0 to disable them. */
#define SHELL_I2C_IS_LOGGING_ENABLED 1

//-------------------------------------------------------------------------------------------------
// Private functions
//-------------------------------------------------------------------------------------------------
/** Convert a numerical value typed by the user to its binary representation.
 * @param Pointer_String The numeric value, it may be prefixed by the letter 'h' to indicate that this is a hexadecimal number. This string is not zero terminated because it is directly pointing to the command line string.
 * @param Length The length of the numeric value string.
 * @param Pointer_Binary On output, contain the converted number.
 * @return 0 on success,
 * @return 1 if an error occurred.
 */
static unsigned char ShellCommandI2CConvertNumericalArgumentToBinary(char *Pointer_String, unsigned char Length, unsigned long *Pointer_Binary)
{
	char Terminating_Character;
	unsigned char Return_Value;

	// Make sure there is some string to parse
	if (Pointer_String == NULL)
	{
		LOG(SHELL_I2C_IS_LOGGING_ENABLED, "Error : the provided string is NULL.");
		return 1;
	}
	if (Length == 0)
	{
		LOG(SHELL_I2C_IS_LOGGING_ENABLED, "Error : the string length is 0.");
		return 1;
	}

	// To avoid parsing the whole string and copying it to a temporary buffer, temporarily just replace the arguments separating character right after the last string character by a terminating character
	Terminating_Character = Pointer_String[Length]; // If this string was the last argument, there is still the zero character terminating the command line string, so we are not accessing an out-of-bound byte here
	Pointer_String[Length] = 0;

	// Handle a hexadecimal number
	if (*Pointer_String == 'h')
	{
		// Bypass the 'h'
		Pointer_String++;
		if (Length == 1)
		{
			LOG(SHELL_I2C_IS_LOGGING_ENABLED, "Error : no number digits provided in the hexadecimal number string.");
			return 1;
		}

		LOG(SHELL_I2C_IS_LOGGING_ENABLED, "Converting the hexadecimal number string \"%s\" to binary.", Pointer_String);
		Return_Value = UtilityConvertHexadecimalToBinary(Pointer_String, Pointer_Binary);
	}
	// Handle a decimal number
	else
	{
		// TODO
		Return_Value = 1;
	}

	// Restore the string terminating character to restore the command line string as we got it
	Pointer_String[Length] = Terminating_Character;

	LOG(SHELL_I2C_IS_LOGGING_ENABLED, "Return value : %u.", Return_Value);
	return Return_Value;
}

//-------------------------------------------------------------------------------------------------
// Public functions
//-------------------------------------------------------------------------------------------------
void ShellCommandI2CCallback(char *Pointer_String_Arguments)
{
	/** The maximum amount of commands that can be read from the command line. */
	#define MAXIMUM_COMMANDS_COUNT 16

	/** All supported command types. */
	typedef enum
	{
		I2C_COMMAND_TYPE_GENERATE_START,
		I2C_COMMAND_TYPE_GENERATE_STOP,
		I2C_COMMAND_TYPE_READ,
		I2C_COMMAND_TYPE_WRITE
	} TI2CCommandType;

	/** Efficiently store the command parameters. */
	typedef struct
	{
		TI2CCommandType Type;
		union
		{
			unsigned long Bytes_Count; //!< For a read operation, how many bytes to read.
			unsigned char Data; //!< For a write operation, the data byte to write.
		};
	} TI2CCommand;

	TI2CCommand Commands[MAXIMUM_COMMANDS_COUNT], *Pointer_Command = Commands;
	unsigned char Commands_Count = 0, Length = 0;
	unsigned long Value;

	// Parse all commands to validate the command line syntax
	while (*Pointer_String_Arguments != 0)
	{
		Pointer_String_Arguments = ShellExtractNextToken(Pointer_String_Arguments, &Length);
		if (Pointer_String_Arguments == NULL) break;
		LOG(SHELL_I2C_IS_LOGGING_ENABLED, "Command (not zero terminated) : \"%s\".", Pointer_String_Arguments);

		// Parse the next command
		switch (*Pointer_String_Arguments)
		{
			case '[':
				LOG(SHELL_I2C_IS_LOGGING_ENABLED, "Found an \"I2C START\" command.");
				Pointer_Command->Type = I2C_COMMAND_TYPE_GENERATE_START;
				break;

			case ']':
				LOG(SHELL_I2C_IS_LOGGING_ENABLED, "Found an \"I2C STOP\" command.");
				Pointer_Command->Type = I2C_COMMAND_TYPE_GENERATE_STOP;
				break;

			case 'r':
				LOG(SHELL_I2C_IS_LOGGING_ENABLED, "Found an \"I2C READ\" command, parsing it.");

				// Make sure that the bytes count was provided to the read command
				if (Length == 1)
				{
					USBCommunicationsWriteString("\r\nError : please provide the amount of bytes to read with the \"r\" command.");
					return;
				}

				// Convert the bytes count to binary
				if (ShellCommandI2CConvertNumericalArgumentToBinary(Pointer_String_Arguments + 1, Length - 1, &Value) != 0) // Add one to bypass the 'r' character
				{
					USBCommunicationsWriteString("\r\nError : the bytes count argument provided to the read command is invalid.");
					return;
				}

				// Fill the command
				LOG(SHELL_I2C_IS_LOGGING_ENABLED, "Asked to read %lu bytes.", Value);
				Pointer_Command->Type = I2C_COMMAND_TYPE_READ;
				Pointer_Command->Bytes_Count = Value;
				break;
		}

		// Go to the next available command slot
		Commands_Count++;
		if (Commands_Count > MAXIMUM_COMMANDS_COUNT)
		{
			USBCommunicationsWriteString("\r\nError : the maximum amount of commands has been reached.");
			return;
		}
	}

	// Tell the user that no command was provided
	if (Commands_Count == 0)
	{
		USBCommunicationsWriteString("\r\nNo I2C command was given.");
		return;
	}
	LOG(SHELL_I2C_IS_LOGGING_ENABLED, "Parsed %u commands.", Commands_Count);

	// TODO
}

void ShellCommandI2CConfigureCallback(char *Pointer_String_Arguments)
{
	unsigned char Length = 0;
	TMSSPI2CFrequency Frequency;

	// Determine the bus frequency
	Pointer_String_Arguments = ShellExtractNextToken(Pointer_String_Arguments, &Length);
	if (Pointer_String_Arguments == NULL)
	{
		USBCommunicationsWriteString("\r\nError : could not find the bus frequency argument.");
		return;
	}
	if (ShellCompareTokenWithString(Pointer_String_Arguments, "100khz", Length) == 0) Frequency = MSSP_I2C_FREQUENCY_100KHZ;
	else if (ShellCompareTokenWithString(Pointer_String_Arguments, "400khz", Length) == 0) Frequency = MSSP_I2C_FREQUENCY_400KHZ;
	else
	{
		USBCommunicationsWriteString("\r\nError : unsupported bus frequency argument. The allowed arguments are \"100khz\" and \"400khz\".");
		return;
	}

	MSSPI2CSetFrequency(Frequency);
	USBCommunicationsWriteString("\r\nSuccess.");
}

void ShellCommandI2CScanCallback(char __attribute__((unused)) *Pointer_String_Arguments)
{
	unsigned char i, Result;
	char String_Temporary[32];

	// Configure the I2C interface
	MSSPSetFunctioningMode(MSSP_FUNCTIONING_MODE_I2C);

	// Ignore the I2C General Call Address of value 0, otherwise we would not know which slave answered
	for (i = 1; i != 127; i++)
	{
		// Send each slave address one at a time
		MSSPI2CGenerateStart();
		Result = MSSPI2CWriteByte((unsigned char) (i << 1) | MSSP_I2C_OPERATION_READ);
		MSSPI2CGenerateStop();

		// Did the slave answered ?
		if (Result == 0)
		{
			snprintf(String_Temporary, sizeof(String_Temporary), "\r\nAddress 0x%02X answered.", i);
			USBCommunicationsWriteString(String_Temporary);
		}
	}
}
