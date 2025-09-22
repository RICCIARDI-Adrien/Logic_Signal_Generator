/** @file Shell_Commands.h
 * Gather all available shell commands, which are implemented in various files.
 * @author Adrien RICCIARDI
 */
#ifndef H_SHELL_COMMANDS_H
#define H_SHELL_COMMANDS_H

//-------------------------------------------------------------------------------------------------
// Constants
//-------------------------------------------------------------------------------------------------
/** How many commands are listed in the Shell_Commands array. */
#define SHELL_COMMANDS_COUNT 7 // The sizeof() operator can't be used on the array as the array is declared in a separate C file

//-------------------------------------------------------------------------------------------------
// Types
//-------------------------------------------------------------------------------------------------
/** The command code.
 * @param Pointer_String_Arguments The remaining arguments provided on the command line.
 */
typedef void (*TShellCommandCallback)(char *Pointer_String_Arguments);

/** A shell command description. */
typedef struct
{
	const char *Pointer_String_Command;
	const char *Pointer_String_Description;
	TShellCommandCallback Command_Callback;
} TShellCommand;

//-------------------------------------------------------------------------------------------------
// Variables
//-------------------------------------------------------------------------------------------------
/** Hold all existing shell commands. */
extern const TShellCommand Shell_Commands[SHELL_COMMANDS_COUNT];

//-------------------------------------------------------------------------------------------------
// Functions
//-------------------------------------------------------------------------------------------------
/** Implement the "help" shell command.
 * @param Pointer_String_Arguments The command line arguments.
 */
void ShellCommandHelpCallback(char *Pointer_String_Arguments);

/** Implement the "i2c" shell command.
 * @param Pointer_String_Arguments The command line arguments.
 */
void ShellCommandI2CCallback(char *Pointer_String_Arguments);

/** Implement the "i2c-configure" shell command.
 * @param Pointer_String_Arguments The command line arguments.
 */
void ShellCommandI2CConfigureCallback(char *Pointer_String_Arguments);

/** Implement the "i2c-scan" shell command.
 * @param Pointer_String_Arguments The command line arguments.
 */
void ShellCommandI2CScanCallback(char *Pointer_String_Arguments);

/** Implement the "pinout" shell command.
 * @param Pointer_String_Arguments The command line arguments.
 */
void ShellCommandPinoutCallback(char *Pointer_String_Arguments);

/** Implement the "spi" shell command.
 * @param Pointer_String_Arguments The command line arguments.
 */
void ShellCommandSPICallback(char *Pointer_String_Arguments);

/** Implement the "spi-configure" shell command.
 * @param Pointer_String_Arguments The command line arguments.
 */
void ShellCommandSPIConfigureCallback(char *Pointer_String_Arguments);

#endif
