/** @file USB_Core.h
 * All USB-generic code not related to a specific class.
 * @author Adrien RICCIARDI
 */
#ifndef H_USB_CORE_H
#define H_USB_CORE_H

//-------------------------------------------------------------------------------------------------
// Functions
//-------------------------------------------------------------------------------------------------
/** Configure the USB peripheral for full-speed operations and attach the device to the bus. */
void USBCoreInitialize(void);

#endif
