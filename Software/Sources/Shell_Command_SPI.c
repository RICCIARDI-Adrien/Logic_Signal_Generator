/** @file Shell_Command_SPI.c
 * Implement all SPI-related shell commands.
 * @author Adrien RICCIARDI
 */
#include <MSSP.h>
#include <Shell.h>
#include <Shell_Commands.h>
#include <USB_Communications.h>

//-------------------------------------------------------------------------------------------------
// Public functions
//-------------------------------------------------------------------------------------------------
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
