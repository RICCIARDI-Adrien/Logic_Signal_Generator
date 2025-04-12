/** @file Main.c
 * Logic signal generator entry point and main loop.
 * @author Adrien RICCIARDI
 */
#include <xc.h>

//-------------------------------------------------------------------------------------------------
// Microcontroller configuration
//-------------------------------------------------------------------------------------------------
// CONFIG1L register
#pragma config CPUDIV = NOCLKDIV, CFGPLLEN = ON, PLLSEL = PLL3X // CPU uses system clock with no division, always enable the PLL, select a 3x clock multiplier factor for the PLL
// CONFIG1H register
#pragma config IESO = OFF, FCMEN = OFF, PCLKEN = ON, FOSC = INTOSCIO // Disable unneeded oscillator switchover, disable fail-safe clock monitor, always enable the primary oscillator, select the internal oscillator as the primary oscillator
// CONFIG2L register
#pragma config nLPBOR = OFF, BORV = 285, BOREN = SBORDIS, nPWRTEN = ON // Disable the low-power brown-out reset, enable the brown-out reset with the highest available voltage, enable the power up timer
// CONFIG2H register
#pragma config WDTEN = OFF // Disable the watchdog timer
// CONFIG3H register
#pragma config MCLRE = ON, SDOMX = RB3, PBADEN = OFF, CCP2MX = RC1 // Enable /MCLR, keep the SPI SDO pin on port B, configure the port A pins as digital I/Os, move the CCP2 pin to port C
// CONFIG4L register
#pragma config DEBUG = OFF, XINST = OFF, ICPRT = OFF, LVP = ON, STVREN = ON // Disable the debugger, disable the extended instruction set, disable the in-circuit debug/programming port, enable the low-voltage programming, stack overflow or underflow will reset the MCU
// CONFIG5L register
#pragma config CP3 = OFF, CP2 = OFF, CP1 = OFF, CP0 = OFF // Disable all code protections
// CONFIG5H register
#pragma config CPD = OFF, CPB = OFF // Disable data EEPROM code protection, disable boot block code protection
// CONFIG6L register
#pragma config WRT3 = ON, WRT2 = ON, WRT1 = ON, WRT0 = ON // Enable blocks write protection
// CONFIG6H register
#pragma config WRTD = OFF, WRTB = ON, WRTC = ON // Allow to write to the EEPROM, prevent writing to the boot block or the configuration registers
// CONFIG7L register
#pragma config EBTR3 = OFF, EBTR2 = OFF, EBTR1 = OFF, EBTR0 = OFF // Disable blocks read protection
// CONFIG7H register
#pragma config EBTRB = OFF // Disable boot block read protection

//-------------------------------------------------------------------------------------------------
// Public functions
//-------------------------------------------------------------------------------------------------
void main(void)
{
	
}
