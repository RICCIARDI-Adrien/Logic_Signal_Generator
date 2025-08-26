/** @file MSSP.h
 * A simple driver for the Master Synchronous Serial Port peripheral, allowing to use it in SPI or I2C mode.
 * @author Adrien RICCIARDI
 */
#ifndef H_MSSP_H
#define H_MSSP_H

//-------------------------------------------------------------------------------------------------
// Types
//-------------------------------------------------------------------------------------------------
/** All supported I2C bus frequencies. */
typedef enum
{
	MSSP_I2C_FREQUENCY_100KHZ,
	MSSP_I2C_FREQUENCY_400KHZ,
} TMSSPI2CFrequency;

//-------------------------------------------------------------------------------------------------
// Functions
//-------------------------------------------------------------------------------------------------
/** Set the I2C bus frequency.
 * @param Frequency The frequency to set.
 */
void MSSPSetI2CFrequency(TMSSPI2CFrequency Frequency);

#endif
