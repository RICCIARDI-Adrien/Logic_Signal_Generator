/** @file USB_Core.h
 * All USB-generic code not related to a specific class.
 * @author Adrien RICCIARDI
 */
#ifndef H_USB_CORE_H
#define H_USB_CORE_H

#include <xc.h>

//-------------------------------------------------------------------------------------------------
// Constants and macros
//-------------------------------------------------------------------------------------------------
/** Tell whether the USB peripheral interrupt needs to be serviced. */
#define USB_CORE_IS_INTERRUPT_FIRED() PIR3bits.USBIF // No need to check whether the interrupt is enabled in the PIE register, because the USB interrupt is always enabled

//-------------------------------------------------------------------------------------------------
// Functions
//-------------------------------------------------------------------------------------------------
/** Configure the USB peripheral for full-speed operations and attach the device to the bus. */
void USBCoreInitialize(void);

/** Must be called from the interrupt context to handle the USB interrupt. */
void USBCoreInterruptHandler(void);

#endif
