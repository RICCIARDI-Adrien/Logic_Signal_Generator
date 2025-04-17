/** @file UART.h
 * Provide access to the serial port. Currently configure the bus at 921600bit/s 8 N 1.
 * @author Adrien RICCIARDI
 */
#ifndef H_UART_H
#define H_UART_H

//-------------------------------------------------------------------------------------------------
// Functions
//-------------------------------------------------------------------------------------------------
/** Initialize the EUSART module for the required operations. */
void UARTInitialize(void);

/** Send a single byte of data through the serial port.
 * @param Data The byte to send.
 */
void UARTWriteByte(unsigned char Data);

#endif
