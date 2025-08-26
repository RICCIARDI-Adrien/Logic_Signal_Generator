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
void MSSPSetI2CFrequency(TMSSPI2CFrequency Frequency)
{
	MSSP_I2C_Frequency = Frequency;
}
