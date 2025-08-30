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

		// Make sure the completion flag is cleared
		PIR1bits.SSPIF = 0;

		// Enable the peripheral
		SSP1CON1bits.SSPEN = 1;
	}
}

void MSSPI2CSetFrequency(TMSSPI2CFrequency Frequency)
{
	MSSP_I2C_Frequency = Frequency;
}

void MSSPI2CGenerateStart(void)
{
	// Start the start sequence
	SSP1CON2bits.SEN = 1;

	// Wait for the start sequence to terminate
	while (!PIR1bits.SSPIF);
	PIR1bits.SSPIF = 0; // Clear the flag
}

void MSSPI2CGenerateRepeatedStart(void)
{
	// Start the repeated start sequence
	SSP1CON2bits.RSEN = 1;

	// Wait for the repeated start sequence to terminate
	while (!PIR1bits.SSPIF);
	PIR1bits.SSPIF = 0; // Clear the flag
}

void MSSPI2CGenerateStop(void)
{
	// Start the stop sequence
	SSP1CON2bits.PEN = 1;

	// Wait for the stop sequence to terminate
	while (!PIR1bits.SSPIF);
	PIR1bits.SSPIF = 0; // Clear the flag
}

unsigned char MSSPI2CReadByte(unsigned char Is_Reception_Acknowledged)
{
	unsigned char Byte;

	// Clock the bus to receive a byte
	SSP1CON2bits.RCEN = 1;

	// Wait for the reception to terminate
	while (!PIR1bits.SSPIF);
	PIR1bits.SSPIF = 0; // Clear the flag

	// Retrieve the read data
	Byte = SSP1BUF;

	// Acknowledge the received byte
	if (Is_Reception_Acknowledged) SSP1CON2bits.ACKDT = 0;
	else SSP1CON2bits.ACKDT = 1;
	SSP1CON2bits.ACKEN = 1; // Start the acknowledge bit transmission

	// Wait for the acknowledge bit transmission to terminate
	while (!PIR1bits.SSPIF);
	PIR1bits.SSPIF = 0; // Clear the flag

	return Byte;
}

unsigned char MSSPI2CWriteByte(unsigned char Byte)
{
	// Start transmitting the byte
	SSP1BUF = Byte;

	// Wait for the transmission to terminate
	while (!PIR1bits.SSPIF);
	PIR1bits.SSPIF = 0; // Clear the flag

	// Tell whether the slave has acknowledged the transfer
	if (SSP1CON2bits.ACKSTAT == 0) return 0;
	return 1;
}
