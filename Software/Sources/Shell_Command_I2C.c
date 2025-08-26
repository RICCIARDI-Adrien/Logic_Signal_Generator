/** @file Shell_Command_I2C.c
 * Implement all I2C-related shell commands.
 * @author Adrien RICCIARDI
 */
#include <MSSP.h>
#include <Shell.h>
#include <Shell_Commands.h>
#include <USB_Communications.h>

//-------------------------------------------------------------------------------------------------
// Public functions
//-------------------------------------------------------------------------------------------------
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
