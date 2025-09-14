/** @file Shell.c
 * See Shell.h for description.
 * @author Adrien RICCIARDI
 */
#include <Log.h>
#include <Shell.h>
#include <string.h>
#include <USB_Communications.h>
#include <Utility.h>

//-------------------------------------------------------------------------------------------------
// Private constants
//-------------------------------------------------------------------------------------------------
/** Set to 1 to enable the log messages, set to 0 to disable them. */
#define SHELL_IS_LOGGING_ENABLED 1

/** The prompt to display. */
#define SHELL_STRING_PROMPT "> "

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
	USBCommunicationsWriteString("\r\n" SHELL_STRING_PROMPT);

	while (1)
	{
		Character = USBCommunicationsReadCharacter();
		switch (Character)
		{
			// Erase the whole line if any of the following key combination is detected
			case 0x03: // Ctrl+C
			case 0x04: // Ctrl+D
			case 0x15: // Ctrl+U
				// Return the cursor to the beginning of the line, then erase it and display the prompt again
				USBCommunicationsWriteString("\r\033[2K" SHELL_STRING_PROMPT); // This is VT100-specific but pretty fast
				Pointer_String_Command_Line -= Length;
				Length = 0;
				break;

			// Discard any VT100 escape sequence, otherwise pressing some keyboard keys can mess the whole displaying
			case 0x1B: // This is redundant with the check in the switch's default, but keeping the escape here allow for a clearer comment
				break;

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
				// Discard any control code not specifically handled before
				if (Character < ' ') break;

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

char *ShellExtractNextToken(char *Pointer_String_Command_Line, unsigned char *Pointer_Token_Length)
{
	char Character, *Pointer_String_Token_Start;
	unsigned char Length;

	// Do nothing if the provided string is NULL
	if (Pointer_String_Command_Line == NULL)
	{
		*Pointer_Token_Length = 0;
		return NULL;
	}

	// Go to the specified string location in order to bypass a previously found token
	Pointer_String_Command_Line += *Pointer_Token_Length;

	// Remove the potential starting space characters
	while (1)
	{
		// Cache the character access
		Character = *Pointer_String_Command_Line;

		// Is the end of the string reached ?
		if (Character == 0)
		{
			*Pointer_Token_Length = 0;
			return NULL;
		}

		// Discard any space character
		if ((Character != '\t') && (Character != ' ')) break;
		Pointer_String_Command_Line++;
	}

	// The token beginning has been found, now find its end
	Pointer_String_Token_Start = Pointer_String_Command_Line;
	Length = 0;
	while (1)
	{
		// Cache the character access
		Character = *Pointer_String_Command_Line;

		// // Stop at the fist space character or at the end of the string
		if ((Character == 0) || (Character == '\t') || (Character == ' '))
		{
			*Pointer_Token_Length = Length;
			return Pointer_String_Token_Start;
		}

		// Prepare for the next character
		Pointer_String_Command_Line++;
		Length++;
	}
}

unsigned char ShellProcessCommand(char *Pointer_String_Command_Line)
{
	char *Pointer_String_Command;
	unsigned char Token_Length = 0, i;
	const TShellCommand *Pointer_Commands = Shell_Commands;

	// The first word is the command itself
	Pointer_String_Command = ShellExtractNextToken(Pointer_String_Command_Line, &Token_Length);

	// Try to match any known command
	for (i = 0; i < SHELL_COMMANDS_COUNT; i++)
	{
		// Is it the right command ?
		if (ShellCompareTokenWithString(Pointer_String_Command, (char *) Pointer_Commands->Pointer_String_Command, Token_Length) == 0)
		{
			LOG(SHELL_IS_LOGGING_ENABLED, "Found the matching command \"%s\", executing it.", Pointer_Commands->Pointer_String_Command);

			// Run the command
			if (Pointer_Commands->Command_Callback == NULL)
			{
				LOG(SHELL_IS_LOGGING_ENABLED, "Error : no command callback is provided, aborting.");
				return 2;
			}
			// Provide the arguments list that point right after the command
			Pointer_Commands->Command_Callback(Pointer_String_Command + Token_Length);

			return 0;
		}

		Pointer_Commands++;
	}

	// No matching command was found
	return 1;
}

unsigned char ShellCompareTokenWithString(char *Pointer_String_Token, char *Pointer_String_To_Compare, unsigned char Token_Length)
{
	unsigned char Length;

	// Make sure the provided strings are valid
	if (Pointer_String_Token == NULL)
	{
		LOG(SHELL_IS_LOGGING_ENABLED, "Error : the token string is NULL.");
		return 2;
	}
	if (Pointer_String_To_Compare == NULL)
	{
		LOG(SHELL_IS_LOGGING_ENABLED, "Error : the string to compare is NULL.");
		return 2;
	}

	// Also make sure that the string length is identical, otherwise two strings that begin the same could be mistakenly told as equal
	Length = (unsigned char) strlen(Pointer_String_To_Compare);
	if (Length != Token_Length) return 1;

	// Eventually compare the two strings
	if (strncmp(Pointer_String_Token, Pointer_String_To_Compare, Token_Length) == 0) return 0;
	return 1;
}

unsigned char ShellConvertNumericalArgumentToBinary(char *Pointer_String, unsigned char Length, unsigned long *Pointer_Binary)
{
	char Terminating_Character;
	unsigned char Return_Value = 1;

	// Make sure there is some string to parse
	if (Pointer_String == NULL)
	{
		LOG(SHELL_IS_LOGGING_ENABLED, "Error : the provided string is NULL.");
		return 1;
	}
	if (Length == 0)
	{
		LOG(SHELL_IS_LOGGING_ENABLED, "Error : the string length is 0.");
		return 1;
	}

	// To avoid parsing the whole string and copying it to a temporary buffer, temporarily just replace the arguments separating character right after the last string character by a terminating character
	Terminating_Character = Pointer_String[Length]; // If this string was the last argument, there is still the zero character terminating the command line string, so we are not accessing an out-of-bound byte here
	Pointer_String[Length] = 0;

	// Handle a hexadecimal number
	if (*Pointer_String == 'h')
	{
		// Is there some data following the 'h' ?
		if (Length == 1)
		{
			LOG(SHELL_IS_LOGGING_ENABLED, "Error : no number digits provided in the hexadecimal number string.");
			goto Exit;
		}

		LOG(SHELL_IS_LOGGING_ENABLED, "Converting the hexadecimal number string \"%s\" to binary.", Pointer_String + 1); // Bypass the 'h'
		Return_Value = UtilityConvertHexadecimalNumberToBinary(Pointer_String + 1, Pointer_Binary); // Bypass the 'h'
	}
	// Handle a decimal number
	else
	{
		LOG(SHELL_IS_LOGGING_ENABLED, "Converting the decimal number string \"%s\" to binary.", Pointer_String);
		Return_Value = UtilityConvertDecimalNumberToBinary(Pointer_String, Pointer_Binary);
	}

Exit:
	// Restore the string terminating character to restore the command line string as we got it
	Pointer_String[Length] = Terminating_Character;

	LOG(SHELL_IS_LOGGING_ENABLED, "Return value : %u.", Return_Value);
	return Return_Value;
}

void ShellDisplayDataDump(unsigned long Starting_Address, unsigned char *Pointer_Data, unsigned char Data_Bytes_Count)
{
	#define MAXIMUM_DATA_BYTES_PER_LINE 16

	unsigned char Chunk_Size, i, j, *Pointer_Data_Beginning, Data;
	char String_Line[81], *Pointer_String, Character; // Use the same line format as `hexdump -C`, which is 78 characters long, append the CRLF sequence, then add space for the terminating zero

	while (Data_Bytes_Count > 0)
	{
		// Determine the maximum amount of data to display on the line
		if (Data_Bytes_Count >= MAXIMUM_DATA_BYTES_PER_LINE) Chunk_Size = MAXIMUM_DATA_BYTES_PER_LINE;
		else Chunk_Size = Data_Bytes_Count;

		// Put the address at the beginning of the string
		Pointer_String = String_Line;
		sprintf(Pointer_String, "%08lX  ", Starting_Address);
		Pointer_String += 10;

		// Append the dumped data
		Pointer_Data_Beginning = Pointer_Data;
		for (i = 0; i < Chunk_Size; i++)
		{
			// Dump one byte at a time
			sprintf(Pointer_String, "%02X ", *Pointer_Data);
			Pointer_String += 3;
			Pointer_Data++;

			// Add an extra separating space after displaying 8 bytes to make the reading easier
			if (i == 7)
			{
				*Pointer_String = ' ';
				Pointer_String++;
			}
		}

		// Fill the space remaining between the hexadecimal dump and the ASCII dump (if any)
		for (; i < MAXIMUM_DATA_BYTES_PER_LINE; i++)
		{
			// Each hexadecimal dump is made of 2 characters followed by a space
			for (j = 0; j < 3; j++)
			{
				*Pointer_String = ' ';
				Pointer_String++;
			}

			// Take also into account the extra space separating the dumped hexadecimal values into two groups of 8 data
			if (i == 7)
			{
				*Pointer_String = ' ';
				Pointer_String++;
			}
		}

		// Append the characters that start the ASCII dump section
		*Pointer_String = ' ';
		Pointer_String++;
		*Pointer_String = '|';
		Pointer_String++;

		// Dump the same characters in ASCII mode
		Pointer_Data = Pointer_Data_Beginning;
		for (i = 0; i < Chunk_Size; i++)
		{
			// Cache the data value
			Data = *Pointer_Data;
			Pointer_Data++;

			// Make sure only printable characters are shown
			if ((Data >= ' ') && (Data <= '~')) Character = Data;
			else Character = '.';

			// Append the character
			*Pointer_String = Character;
			Pointer_String++;
		}

		// Fill the space remaining until the end of the ASCII dump area (if any)
		for (; i < MAXIMUM_DATA_BYTES_PER_LINE; i++)
		{
			*Pointer_String = ' ';
			Pointer_String++;
		}

		// Terminate the ASCII dump section and the string
		*Pointer_String = '|';
		Pointer_String++;
		*Pointer_String = '\r';
		Pointer_String++;
		*Pointer_String = '\n';
		Pointer_String++;
		*Pointer_String = 0;

		// Display the result
		USBCommunicationsWriteString(String_Line);

		// Update the pointers for the next iteration
		Starting_Address += Chunk_Size;
		Data_Bytes_Count -= Chunk_Size;
	}
}
