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

/** The SPI bus frequency. */
static TMSSPSPIFrequency MSSP_SPI_Frequency = MSSP_SPI_FREQUENCY_100KHZ;
/** The SPI bus polarity and phase mode. */
static TMSSPSPIMode MSSP_SPI_Mode = MSSP_SPI_MODE_0;

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
	}
	// SPI mode
	else
	{
		// Configure the pins
		// Set the required directions
		TRISBbits.TRISB0 = 1; // SDI must be an input
		TRISBbits.TRISB1 = 0; // SCK must be an output
		TRISBbits.TRISB2 = 0; // /CS GPIO must be an output
		TRISBbits.TRISB3 = 0; // SDO must be an output
		// Enable the digital input buffers
		ANSELB &= 0xF0;

		// Make sure that the slave device is not selected
		MSSPSPISelectSlave(0);

		// Set the configured bus frequency
		SSP1ADD = MSSP_SPI_Frequency;

		// Configure the peripheral
		SSP1STAT = 0; // Sample the input data at the middle of the output time
		if ((MSSP_SPI_Mode == MSSP_SPI_MODE_0) || (MSSP_SPI_Mode == MSSP_SPI_MODE_2)) SSP1STATbits.CKE = 1; // Configure the clock phase
		SSP1CON1 = 0x0A; // Select the SPI master mode with the clock configured in SSP1ADD
		if ((MSSP_SPI_Mode == MSSP_SPI_MODE_2) || (MSSP_SPI_Mode == MSSP_SPI_MODE_3)) SSP1CON1bits.CKP = 1; // Configure the clock polarity
		SSP1CON2 = 0;
		SSP1CON3 = 0;
	}

	// Make sure the completion flag is cleared
	PIR1bits.SSPIF = 0;

	// Enable the peripheral
	SSP1CON1bits.SSPEN = 1;
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

void MSSPSPISetFrequency(TMSSPSPIFrequency Frequency)
{
	MSSP_SPI_Frequency = Frequency;
}

void MSSPSPISetMode(TMSSPSPIMode Mode)
{
	MSSP_SPI_Mode = Mode;
}

void MSSPSPISelectSlave(unsigned char Is_Asserted)
{
	// The /CS signal is active low
	if (Is_Asserted) LATBbits.LATB2 = 0;
	else LATBbits.LATB2 = 1;
}

unsigned char MSSPSPITransmitByte(unsigned char Byte)
{
	// Start transmitting the byte
	SSP1BUF = Byte;

	// Wait for the transmission to terminate
	while (!PIR1bits.SSPIF);
	PIR1bits.SSPIF = 0; // Clear the flag

	// Retrieve the data sent by the slave
	return SSP1BUF;
}
