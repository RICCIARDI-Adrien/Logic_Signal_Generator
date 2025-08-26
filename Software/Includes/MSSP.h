/** @file MSSP.h
 * A simple driver for the Master Synchronous Serial Port peripheral, allowing to use it in SPI or I2C mode.
 * @author Adrien RICCIARDI
 */
#ifndef H_MSSP_H
#define H_MSSP_H

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

#endif
