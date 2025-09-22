/** @file Shell_Command_SPI.c
 * Implement all SPI-related shell commands.
 * @author Adrien RICCIARDI
 */
#include <Log.h>
#include <MSSP.h>
#include <Shell.h>
#include <Shell_Commands.h>
#include <USB_Communications.h>

//-------------------------------------------------------------------------------------------------
// Private constants
//-------------------------------------------------------------------------------------------------
/** Set to 1 to enable the log messages, set to 0 to disable them. */
#define SHELL_SPI_IS_LOGGING_ENABLED 1

//-------------------------------------------------------------------------------------------------
// Public functions
//-------------------------------------------------------------------------------------------------
void ShellCommandSPICallback(char *Pointer_String_Arguments)
{
	/** The maximum amount of commands that can be read from the command line. */
	#define MAXIMUM_COMMANDS_COUNT 16

	/** All supported command types. */
	typedef enum
	{
		SPI_COMMAND_TYPE_SELECT_SLAVE,
		SPI_COMMAND_TYPE_DESELECT_SLAVE,
		SPI_COMMAND_TYPE_SINGLE_BYTE_TRANSFER,
		SPI_COMMAND_TYPE_MULTIPLE_BYTES_TRANSFER
	} TSPICommandType;

	/** Efficiently store the command parameters. */
	typedef struct
	{
		TSPICommandType Type;
		union
		{
			unsigned long Bytes_Count; //!< For a multiple bytes transfer operation, how many bytes to read.
			unsigned char Data; //!< For a single byte transfer operation, the data byte to write.
		};
	} TSPICommand;

	TSPICommand Commands[MAXIMUM_COMMANDS_COUNT], *Pointer_Command = Commands;
	unsigned char Commands_Count = 0, Length = 0;
	unsigned long Value;

	// Parse all commands to validate the command line syntax
	while (*Pointer_String_Arguments != 0)
	{
		Pointer_String_Arguments = ShellExtractNextToken(Pointer_String_Arguments, &Length);
		if (Pointer_String_Arguments == NULL) break;
		LOG(SHELL_SPI_IS_LOGGING_ENABLED, "Command (not zero terminated) : \"%s\".", Pointer_String_Arguments);

		// Parse the next command
		switch (*Pointer_String_Arguments)
		{
			case '[':
				LOG(SHELL_SPI_IS_LOGGING_ENABLED, "Found a \"SPI select slave\" command.");
				Pointer_Command->Type = SPI_COMMAND_TYPE_SELECT_SLAVE;
				break;

			case ']':
				LOG(SHELL_SPI_IS_LOGGING_ENABLED, "Found a \"SPI deselect slave\" command.");
				Pointer_Command->Type = SPI_COMMAND_TYPE_DESELECT_SLAVE;
				break;

			case 't':
				LOG(SHELL_SPI_IS_LOGGING_ENABLED, "Found a \"SPI multiple bytes transfer\" command, parsing it.");

				// Make sure that the bytes count was provided to the read command
				if (Length == 1)
				{
					USBCommunicationsWriteString("\r\nError : please provide the amount of bytes to transfer with the \"t\" command.");
					return;
				}

				// Convert the bytes count to binary
				if (ShellConvertNumericalArgumentToBinary(Pointer_String_Arguments + 1, Length - 1, &Value) != 0) // Add one to bypass the 't' character
				{
					USBCommunicationsWriteString("\r\nError : the bytes count argument provided to the transfer command is invalid.");
					return;
				}

				// Fill the command
				LOG(SHELL_SPI_IS_LOGGING_ENABLED, "Asked to transfer %lu bytes.", Value);
				Pointer_Command->Type = SPI_COMMAND_TYPE_MULTIPLE_BYTES_TRANSFER;
				Pointer_Command->Bytes_Count = Value;
				break;

			default:
				LOG(SHELL_SPI_IS_LOGGING_ENABLED, "Trying to find a single byte transfer command.");

				// Convert the bytes count to binary
				if (ShellConvertNumericalArgumentToBinary(Pointer_String_Arguments, Length, &Value) != 0)
				{
					USBCommunicationsWriteString("\r\nError : a command is invalid.");
					return;
				}

				// Only bytes are allowed
				if (Value > 255)
				{
					USBCommunicationsWriteString("\r\nError : only bytes are allowed as a single byte transfer command data, make sure the value is in range [0,255].");
					return;
				}

				// Fill the command
				Pointer_Command->Type = SPI_COMMAND_TYPE_SINGLE_BYTE_TRANSFER;
				Pointer_Command->Data = (unsigned char) Value;
				LOG(SHELL_SPI_IS_LOGGING_ENABLED, "Found a single byte transfer command with value 0x%02X.",  Pointer_Command->Data);
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
		USBCommunicationsWriteString("\r\nNo SPI command was given.");
		return;
	}
	LOG(SHELL_SPI_IS_LOGGING_ENABLED, "Parsed %u commands, now executing them.", Commands_Count);

	// TODO
}

void ShellCommandSPIConfigureCallback(char *Pointer_String_Arguments)
{
	unsigned char Length = 0;
	TMSSPSPIFrequency Frequency;
	TMSSPSPIMode Mode;

	// Determine the bus frequency
	Pointer_String_Arguments = ShellExtractNextToken(Pointer_String_Arguments, &Length);
	if (Pointer_String_Arguments == NULL)
	{
		USBCommunicationsWriteString("\r\nError : could not find the bus frequency argument.");
		return;
	}
	if (ShellCompareTokenWithString(Pointer_String_Arguments, "50khz", Length) == 0) Frequency = MSSP_SPI_FREQUENCY_50KHZ;
	else if (ShellCompareTokenWithString(Pointer_String_Arguments, "100khz", Length) == 0) Frequency = MSSP_SPI_FREQUENCY_100KHZ;
	else if (ShellCompareTokenWithString(Pointer_String_Arguments, "500khz", Length) == 0) Frequency = MSSP_SPI_FREQUENCY_500KHZ;
	else if (ShellCompareTokenWithString(Pointer_String_Arguments, "1mhz", Length) == 0) Frequency = MSSP_SPI_FREQUENCY_1MHZ;
	else if (ShellCompareTokenWithString(Pointer_String_Arguments, "2mhz", Length) == 0) Frequency = MSSP_SPI_FREQUENCY_2MHZ;
	else
	{
		USBCommunicationsWriteString("\r\nError : unsupported bus frequency argument. See the command help for a list of the allowed frequencies.");
		return;
	}

	// Determine the mode
	Pointer_String_Arguments = ShellExtractNextToken(Pointer_String_Arguments, &Length);
	if (Pointer_String_Arguments == NULL)
	{
		USBCommunicationsWriteString("\r\nError : could not find the mode argument.");
		return;
	}
	if (ShellCompareTokenWithString(Pointer_String_Arguments, "mode0", Length) == 0) Mode = MSSP_SPI_MODE_0;
	else if (ShellCompareTokenWithString(Pointer_String_Arguments, "mode1", Length) == 0) Mode = MSSP_SPI_MODE_1;
	else if (ShellCompareTokenWithString(Pointer_String_Arguments, "mode2", Length) == 0) Mode = MSSP_SPI_MODE_2;
	else if (ShellCompareTokenWithString(Pointer_String_Arguments, "mode3", Length) == 0) Mode = MSSP_SPI_MODE_3;
	else
	{
		USBCommunicationsWriteString("\r\nError : unsupported mode argument. See the command help for a list of the allowed modes.");
		return;
	}

	// Apply the new settings
	MSSPSPISetFrequency(Frequency);
	MSSPSPISetMode(Mode);
	USBCommunicationsWriteString("\r\nSuccess.");
}
