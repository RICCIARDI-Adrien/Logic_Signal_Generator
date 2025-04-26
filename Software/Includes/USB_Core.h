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

/** The size in byte of any endpoint buffer (this conforms to the USB 2.0 specifications for full-speed devices). */
#define USB_CORE_ENDPOINT_PACKETS_SIZE 64

//-------------------------------------------------------------------------------------------------
// Functions
//-------------------------------------------------------------------------------------------------
/** Configure the USB peripheral for full-speed operations and attach the device to the bus. */
void USBCoreInitialize(void);

/** Configure the specified endpoint out buffer for an upcoming reception of data from the host.
 * @param Endpoint_ID The endpoint number (any endpoint other than 0 must have been enabled in the device descriptors).
 * @note The maximum endpoint packet size is always set to USB_CORE_ENDPOINT_PACKETS_SIZE.
 */
void USBCorePrepareForOutTransfer(unsigned char Endpoint_ID);

/** Must be called from the interrupt context to handle the USB interrupt. */
void USBCoreInterruptHandler(void);

#endif
