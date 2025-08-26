/** @file MSSP.c
 * See MSSP.h for description.
 * @author Adrien RICCAIRDI
 */
#include <MSSP.h>
#include <xc.h>

//-------------------------------------------------------------------------------------------------
// Private variables
//-------------------------------------------------------------------------------------------------
/** The I2C bus frequency. */
static TMSSPI2CFrequency MSSP_I2C_Frequency = MSSP_I2C_FREQUENCY_100KHZ;

//-------------------------------------------------------------------------------------------------
// Public functions
//-------------------------------------------------------------------------------------------------
void MSSPSetFunctioningMode(TMSSPFunctioningMode Mode)
{
	// Disable the peripheral before changing its configuration
	SSP1CON1bits.SSPEN = 0;

	if (Mode == MSSP_FUNCTIONING_MODE_I2C)
	{
		// Set the pins as inputs so the MSSP module can take control of them
		TRISBbits.TRISB0 = 1;
		TRISBbits.TRISB1 = 1;
		// Enable the digital input buffers for the two pins
		ANSELBbits.ANSB0 = 0;
		ANSELBbits.ANSB1 = 0;

		// Set the configured bus frequency
		SSP1ADD = MSSP_I2C_Frequency;

		// Configure the peripheral
		SSP1STAT = 0; // Disable the SMBus input logic threshold
		if (MSSP_I2C_Frequency == MSSP_I2C_FREQUENCY_100KHZ) SSP1STATbits.SMP = 1; // Disable the slew rate control in low speed mode
		SSP1CON1 = 0x08; // Select the I2C master mode
		SSP1CON2 = 0; // Reset the register
		SSP1CON3 = 0x04; // Enable the 300ns hold time on SDA after the falling edge of SCL, this should improve the reliability on busses with large capacitance

		// Enable the peripheral
		SSP1CON1bits.SSPEN = 1;
	}
}

void MSSPI2CSetFrequency(TMSSPI2CFrequency Frequency)
{
	MSSP_I2C_Frequency = Frequency;
}
