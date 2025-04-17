/** @file UART.c
 * See UART.h for description.
 * @author Adrien RICCIARDI
 */
#include <UART.h>
#include <xc.h>

//-------------------------------------------------------------------------------------------------
// Public functions
//-------------------------------------------------------------------------------------------------
void UARTInitialize(void)
{
	// Set the baudrate to 921600 bits/s
	SPBRGH1 = 0;
	SPBRG1 = 12; // Real baud rate is 923077 bits/s (0,0016% deviation)
	BAUDCON1 = 0x08; // Keep default RX and TX signals polarity, select the 16-bit baud rate generator, disable auto-baud detection

	// Configure the module
	TXSTA1 = 0x24; // Select 8-bit transmission and asynchronous mode, enable the transmission, select the high baud rate
	RCSTA1 = 0x90; // Enable the serial port, select 8-bit reception, enable the receiver

	// Configure the pins
	// The pins direction must be set to input
	TRISCbits.TRISC6 = 1;
	TRISCbits.TRISC7 = 1;
	// Disable the analog inputs
	ANSELCbits.ANSC6 = 0;
	ANSELCbits.ANSC7 = 0;
}

void UARTWriteByte(unsigned char Data)
{
	// Wait for the bus to become ready
	while (!PIR1bits.TXIF);

	// Transmit the byte
	TXREG1 = Data;
}
