/** @file Shell.h
 * Provide the base features for a simple text-based interactive shell.
 * @author Adrien RICCIARDI
 */
#ifndef H_SHELL_H
#define H_SHELL_H

#include <Shell_Commands.h>

//-------------------------------------------------------------------------------------------------
// Functions
//-------------------------------------------------------------------------------------------------
/** Block until a command line is entered by the user (note that the returned command line can be empty).
 * @param Pointer_String_Command_Line On output, will contain the command line typed by the user.
 * @param Maximum_Length The command line string buffer size, including the terminating zero character.
 */
void ShellReadCommandLine(char *Pointer_String_Command_Line, unsigned char Maximum_Length);

/** Process a string by discarding the separating characters (mostly space) and find the first meaningful token word.
 * @param Pointer_String_Command_Line On the first call, provide the command line as retrieved with a call to ShellReadCommandLine(). On the following calls, provide the previous result returned by this function call (in order to update the beginning of the next token).
 * @param Pointer_Token_Length On input, contain the length of the previously found token (use 0 for the initial call to this function). On output, contain the length of the newly found token (if any).
 * @return NULL if no token was found before the end of the command line string,
 * @return A pointer to a non zero-terminated string that points to the beginning of the token word. Use the token length to manipulate the token string.
 * @note A token size is currently to limited to 255 bytes.
 */
char *ShellExtractNextToken(char *Pointer_String_Command_Line, unsigned char *Pointer_Token_Length);

/** Determine which command the user has typed in the shell and execute it.
 * @param Pointer_String_Command_Line The command line as retrieved by ShellReadCommandLine().
 * @return 0 if the command was successfully executed,
 * @return 1 if no matching command was found,
 * @return 2 if the command was found but its execution callback was missing.
 */
unsigned char ShellProcessCommand(char *Pointer_String_Command_Line);

/** Allow to easily compare a token (which is a non zero-terminated string) with a classic ASCIIZ string.
 * @param Pointer_String_Token The token string, which may or may not be zero-terminated.
 * @param Pointer_String_To_Compare The string to compare with.
 * @param Token_Length The length of the token string, as reported by the call to ShellExtractNextToken().
 * @return 0 if the two strings are equal (like strlen() would do),
 * @return 1 if the two strings do not match,
 * @return 2 if one or both of the provided string arguments are NULL.
 */
unsigned char ShellCompareTokenWithString(char *Pointer_String_Token, char *Pointer_String_To_Compare, unsigned char Token_Length);

/** Convert a numerical value typed by the user to its binary representation.
 * @param Pointer_String The numeric value, it may be prefixed by the letter 'h' to indicate that this is a hexadecimal number. This string is not zero terminated because it is directly pointing to the command line string read with ShellReadCommandLine().
 * @param Length The length of the numeric value string.
 * @param Pointer_Binary On output, contain the converted number.
 * @return 0 on success,
 * @return 1 if an error occurred.
 */
unsigned char ShellConvertNumericalArgumentToBinary(char *Pointer_String, unsigned char Length, unsigned long *Pointer_Binary);

/** Display a hexadecimal dump of the data followed by an ASCII dump.
 * @param Starting_Address The address value to display at the beginning of the dump.
 * @param Pointer_Data The data bytes to display.
 * @param Data_Bytes_Count How many data bytes to process.
 */
void ShellDisplayDataDump(unsigned long Starting_Address, unsigned char *Pointer_Data, unsigned char Data_Bytes_Count);

#endif
