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
	MSSP_FUNCTIONING_MODE_I2C,
	MSSP_FUNCTIONING_MODE_SPI
} TMSSPFunctioningMode;

/** All supported I2C bus frequencies. The computing formula is Baud_Rate = Fosc / (Fclk * 4) - 1. */
typedef enum : unsigned char
{
	MSSP_I2C_FREQUENCY_100KHZ = 119, // For a 48MHz Fosc
	MSSP_I2C_FREQUENCY_400KHZ = 29, // For a 48MHz Fosc
} TMSSPI2CFrequency;

/** All supported SPI bus frequencies. The computing formula is Baud_Rate = Fosc / (Fclk * 4) - 1. */
typedef enum : unsigned char
{
	MSSP_SPI_FREQUENCY_50KHZ = 239, // For a 48MHz Fosc
	MSSP_SPI_FREQUENCY_100KHZ = 119, // For a 48MHz Fosc
	MSSP_SPI_FREQUENCY_500KHZ = 23, // For a 48MHz Fosc
	MSSP_SPI_FREQUENCY_1MHZ = 11, // For a 48MHz Fosc
	MSSP_SPI_FREQUENCY_2MHZ = 5, // For a 48MHz Fosc
} TMSSPSPIFrequency;

/** All supported SPI polarity and phase modes. */
typedef enum
{
	MSSP_SPI_MODE_0,
	MSSP_SPI_MODE_1,
	MSSP_SPI_MODE_2,
	MSSP_SPI_MODE_3
} TMSSPSPIMode;

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

/** Generate an I2C repeated start sequence and wait for it to complete. */
void MSSPI2CGenerateRepeatedStart(void);

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

/** Set the SPI bus frequency.
 * @param Frequency The frequency to set.
 */
void MSSPSPISetFrequency(TMSSPSPIFrequency Frequency);

/** Configure the polarity and phase mode to use.
 * @param Mode The mode to apply.
 */
void MSSPSPISetMode(TMSSPSPIMode Mode);

/** Assert or deassert the /CS line.
 * @param Is_Asserted Set to 1 to select the slave device, set to 0 to release the slave device.
 */
void MSSPSPISelectSlave(unsigned char Is_Asserted);

/** Send a byte to the slave device and fetch the byte received from the slave at the same time.
 * @param Byte The data to send to the slave.
 * @return The data read from the slave.
 * @note This function purposely does not control the /CS signal, to give more flexibility to create multi transfers.
 */
unsigned char MSSPSPITransmitByte(unsigned char Byte);

#endif
