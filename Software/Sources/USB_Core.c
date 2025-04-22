/** @file USB_Core.c
 * See USB_Core.h
 * @author Adrien RICCIARDI
 */
#include <USB_Core.h>
#include <xc.h>

//-------------------------------------------------------------------------------------------------
// Public functions
//-------------------------------------------------------------------------------------------------
void USBCoreInitialize(void)
{
	// Disable eye test pattern, disable the USB OE monitoring signal, enable the on-chip pull-up, select the full-speed device mode, TODO for now disable ping-pong buffers, this will be a further optimization
	UCFG = 0x14;

	// Enable the packet transfer
	UCON = 0;

	// Enable the USB module and attach the device to the USB bus
	UCONbits.USBEN = 1;
}
