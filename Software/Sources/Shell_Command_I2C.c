/** @file Shell_Command_I2C.c
 * Implement all I2C-related shell commands.
 * @author Adrien RICCIARDI
 */
#include <MSSP.h>
#include <Shell.h>
#include <Shell_Commands.h>
#include <stdio.h>
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
