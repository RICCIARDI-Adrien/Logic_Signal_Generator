/** @file Log.c
 * See Log.h for more description.
 * @author Adrien RICCIARDI
 */
#include <Log.h>
#include <UART.h>

//-------------------------------------------------------------------------------------------------
// Private functions
//-------------------------------------------------------------------------------------------------
#ifdef LOG_ENABLE_LOGGING
	// Implement the XC8 C library putch() function to be able to directly use printf() in the code
	void putch(char Data)
	{
		UARTWriteByte(Data);
	}
#endif
