/** @file MSSP.h
 * A simple driver for the Master Synchronous Serial Port peripheral, allowing to use it in SPI or I2C mode.
 * @author Adrien RICCIARDI
 */
#ifndef H_MSSP_H
#define H_MSSP_H

//-------------------------------------------------------------------------------------------------
// Constants
//-------------------------------------------------------------------------------------------------
/** The least significant bit value of the address byte for a read operation. */
#define MSSP_I2C_OPERATION_READ 0x01
/** The least significant bit value of the address byte for a write operation. */
#define MSSP_I2C_OPERATION_WRITE 0

//-------------------------------------------------------------------------------------------------
// Types
//-------------------------------------------------------------------------------------------------
/** All supported functioning modes. */
typedef enum
{
	MSSP_FUNCTIONING_MODE_I2C
} TMSSPFunctioningMode;

/** All supported I2C bus frequencies. The computing formula is Baud_Rate = Fosc / (Fclk * 4) - 1. */
typedef enum : unsigned char
{
	MSSP_I2C_FREQUENCY_100KHZ = 119, // For a 48MHz Fosc
	MSSP_I2C_FREQUENCY_400KHZ = 29, // For a 48MHz Fosc
} TMSSPI2CFrequency;

//-------------------------------------------------------------------------------------------------
// Functions
//-------------------------------------------------------------------------------------------------
/** Configure the peripheral to work either in I2C master mode or SPI master mode.
 * @param Mode The mode to configure.
 * @note After calling this function, the selected mode is operational and communicate on the bus.
 */
void MSSPSetFunctioningMode(TMSSPFunctioningMode Mode);

/** Set the I2C bus frequency.
 * @param Frequency The frequency to set.
 */
void MSSPI2CSetFrequency(TMSSPI2CFrequency Frequency);

/** Generate an I2C start sequence and wait for it to complete. */
void MSSPI2CGenerateStart(void);

/** Generate an I2C stop sequence and wait for it to complete. */
void MSSPI2CGenerateStop(void);

/** Read a byte of data from the I2C bus.
 * @param Is_Reception_Acknowledged Set to 0 to send a NAK to the slave, or set to 1 to send an ACK to the slave.
 * @return The read byte.
 */
unsigned char MSSPI2CReadByte(unsigned char Is_Reception_Acknowledged);

/** Write a byte on the I2C bus and wait for the transmission to terminate.
 * @return 0 if the slave device acknowledged,
 * @return 1 if the slave device did not acknowledge.
 */
unsigned char MSSPI2CWriteByte(unsigned char Byte);

#endif
