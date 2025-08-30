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

//-------------------------------------------------------------------------------------------------
// Private constants
//-------------------------------------------------------------------------------------------------
/** Set to 1 to enable the log messages, set to 0 to disable them. */
#define SHELL_I2C_IS_LOGGING_ENABLED 1

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
	unsigned char Commands_Count = 0, Length = 0, i, Is_Start_Generated = 0;
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
				if (ShellConvertNumericalArgumentToBinary(Pointer_String_Arguments + 1, Length - 1, &Value) != 0) // Add one to bypass the 'r' character
				{
					USBCommunicationsWriteString("\r\nError : the bytes count argument provided to the read command is invalid.");
					return;
				}

				// Fill the command
				LOG(SHELL_I2C_IS_LOGGING_ENABLED, "Asked to read %lu bytes.", Value);
				Pointer_Command->Type = I2C_COMMAND_TYPE_READ;
				Pointer_Command->Bytes_Count = Value;
				break;

			default:
				LOG(SHELL_I2C_IS_LOGGING_ENABLED, "Trying to find a write command.");

				// Convert the bytes count to binary
				if (ShellConvertNumericalArgumentToBinary(Pointer_String_Arguments, Length, &Value) != 0)
				{
					USBCommunicationsWriteString("\r\nError : a command is invalid.");
					return;
				}

				// Only bytes are allowed
				if (Value > 255)
				{
					USBCommunicationsWriteString("\r\nError : only bytes are allowed as a write command data, make sure the value is in range [0,255].");
					return;
				}

				// Fill the command
				Pointer_Command->Type = I2C_COMMAND_TYPE_WRITE;
				Pointer_Command->Data = (unsigned char) Value;
				LOG(SHELL_I2C_IS_LOGGING_ENABLED, "Found a write command with value 0x%02X.",  Pointer_Command->Data);
				break;
		}

		// Go to the next available command slot
		Commands_Count++;
		if (Commands_Count > MAXIMUM_COMMANDS_COUNT)
		{
			USBCommunicationsWriteString("\r\nError : the maximum amount of commands has been reached.");
			return;
		}
		Pointer_Command++;
	}

	// Tell the user that no command was provided
	if (Commands_Count == 0)
	{
		USBCommunicationsWriteString("\r\nNo I2C command was given.");
		return;
	}
	LOG(SHELL_I2C_IS_LOGGING_ENABLED, "Parsed %u commands, now executing them.", Commands_Count);

	// Configure the I2C interface
	MSSPSetFunctioningMode(MSSP_FUNCTIONING_MODE_I2C);

	// Execute the commands
	Pointer_Command = Commands;
	for (i = 0; i < Commands_Count; i++)
	{
		LOG(SHELL_I2C_IS_LOGGING_ENABLED, "Executing command %u.", i);
		switch (Pointer_Command->Type)
		{
			case I2C_COMMAND_TYPE_GENERATE_START:
				// Differentiate between an I2C start and a repeated start
				if (Is_Start_Generated)
				{
					LOG(SHELL_I2C_IS_LOGGING_ENABLED, "Generating a REPEATED START.");
					MSSPI2CGenerateRepeatedStart();
				}
				else
				{
					LOG(SHELL_I2C_IS_LOGGING_ENABLED, "Generating a START.");
					MSSPI2CGenerateStart();
					Is_Start_Generated = 1;
				}
				break;

			case I2C_COMMAND_TYPE_GENERATE_STOP:
				LOG(SHELL_I2C_IS_LOGGING_ENABLED, "Generating a STOP.");
				MSSPI2CGenerateStop();
				Is_Start_Generated = 0;
				break;
		}

		// Go to the next command
		Pointer_Command++;
	}
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
