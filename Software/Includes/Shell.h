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

#endif
