# Logic Signal Generator

A versatile multi-protocol 1.8V/3.3V/5V logic signal generator powered by a PIC18F25K50.

![Fully assembled device corner](Resources/Fully_Assembled_Device_Corner.jpg)

## Features

* 8 independant channels featuring several standard protocols.
* The microcontroller hardware I2C, UART, SPI and PWM modules are used to provide the signal timings.
* Entirely USB-powered, no need for an external power supply.
* Dedicated power supplies and ground connector to easily power the device under test. The embedded 1.8V and 3.3V power supplies are each able to provide 500mA (these power supplies are derived from the USB power, so the available power could be limited by the USB port).
* On-board switch to quickly select the logic signals output voltage (1.8V, 3.3V or 5V).
* The USB interface is managed by the microcontroller itself and provides a standard USB serial port to the host.
* Simple shell-based text interface through the serial port to interactively configure and define the logic signals to generate.
* The signals are doubled on the output connector to easily connect a logic analyzer.
* PICkit-compatible standard connector to easily program the on-board microcontroller firmware.
* Compact and robust casing designed to bring the signal generator on field.

## Hardware

Bare PCB :
![Bare PCB front view](Resources/Bare_PCB_Top.jpg)
![Bare PCB bottom view](Resources/Bare_PCB_Bottom.jpg)

Assembled PCB :
![Assembled PCB front view](Resources/Assembled_PCB_Top.jpg)
![Assembled PCB bottom view](Resources/Assembled_PCB_Bottom.jpg)

## Casing

The casing is made of 4mm-thick 304 stainless steel.

Bare plates :
![Bare plates](Resources/Bare_Casing_Plates.jpg)

Assembled bottom plate, with PCB and hex spacers :
![Assembled bottom plate](Resources/Assembled_Casing_Bottom.jpg)

Fully assembled device :  
Top view :
![Fully assembled device](Resources/Fully_Assembled_Device_Top.jpg)
IO and power connectors :
![Fully assembled device side IO connector](Resources/Fully_Assembled_Device_Side_IO_Connector.jpg)
USB connector and IO voltage selection switch :
![Fully assembled device side USB connector](Resources/Fully_Assembled_Device_Side_USB_Connector.jpg)
Power and activity LEDs along with microcontroller programming connector :
![Fully assembled device side LEDs](Resources/Fully_Assembled_Device_Side_LEDs.jpg)
