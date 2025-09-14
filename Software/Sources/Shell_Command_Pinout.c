/** @file Shell_Command_Pinout.c
 * Implement the shell "pinout" command.
 * @author Adrien RICCIARDI
 */
#include <Shell_Commands.h>
#include <USB_Communications.h>

//-------------------------------------------------------------------------------------------------
// Public functions
//-------------------------------------------------------------------------------------------------
void ShellCommandPinoutCallback(char __attribute__((unused)) *Pointer_String_Arguments)
{
	USBCommunicationsWriteString("\r\n"
		"I2C :\r\n"
		"  - SCL (clock) : IO 5\r\n"
		"  - SDA (data)  : IO 4\r\n"
		"SPI :\r\n"
		"  - SCLK (clock)            : IO 5\r\n"
		"  - MOSI (data from master) : IO 7\r\n"
		"  - MISO (data from slave)  : IO 4\r\n"
		"  - /CS  (chip select)      : IO 6\r\n"
		"UART :\r\n"
		"  - RX (reception)    : IO 3\r\n"
		"  - TX (transmission) : IO 2"
	);
}
