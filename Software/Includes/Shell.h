/** @file Shell.h
 * Provide the base features for a simple text-based interactive shell.
 * @author Adrien RICCIARDI
 */
#ifndef H_SHELL_H
#define H_SHELL_H

//-------------------------------------------------------------------------------------------------
// Functions
//-------------------------------------------------------------------------------------------------
/** Block until a command line is entered by the user (note that the returned command line can be empty).
 * @param Pointer_String_Command_Line On output, will contain the command line typed by the user.
 * @param Maximum_Length The command line string buffer size, including the terminating zero character.
 */
void ShellReadCommandLine(char *Pointer_String_Command_Line, unsigned char Maximum_Length);

#endif
