# Arduino USB Bridge to Controllerless Graphics LCD

For Arduino MCUs that lack the memory to drive controllerless display in graphics mode, one way around it is if something else is sending the display data, in realtime. We need 9600 bytes x 70 fps = 672KBps ~= 5.4Mbps.

## USB controller on ATmega32u4

On 32u4 there is one peripheral capable of receiving needed amount of data, the USB controller, which has its own independent dual-ported RAM presented to main MCU as FIFOs. 

At 70Hz, we need about 672 bytes per USB microframe (1ms) to keep up with displays demand.

Isochronous endpoint theoretically would be ideal for this as in full-speed mode it can send one transfer (up to 1023 bytes) per microframe. Unfortunately, according to 32u4 datasheet, only endpoint 1 can be up to 256 bytes. The rest are limited to 64-bytes.  

This also eliminates 'Interrupt' endpoints, which are limited to single transfer per microframe, and with that any HID implementation, which is limited to single interrupt OUT endpoint.

Fortunately the 'Bulk' endpoints, which can receive multiple packets per USB microframe, can be configured in dual-bank mode (default in Arduino USB Core) which enables us to process received data while the next packet is handled by the USB peripheral. 

Again, ideally we would just shift-out the data from the USB FIFO directly to the LCD. However, Windows is not an RTOS and there can be delays between transfers, so we need to buffer some amount of data in memory to smooth out delays.

The Arduino code in this directory does exactly that: Reads data from USB endpoint and stores it in SRAM ring-buffer. When the buffer is filled, it starts to shift-out the data to the LCD, while the buffer is being constantly refilled if there is data available in the USB FIFO. It also handles the LCD control lines, as explained elsewhere in this repo. If it runs out of data in the buffer, it cleans up and waits for next start of frame (USB packet beginning with zero) to fill the buffer, without latched FLM bit. After short, period with no data, it gives up and shutdowns the display.

 On the host side, there is a C++ process which reads 'named' shared memory frame buffer (320x240 bytes), applies 'temporal dithering' to the grayscale values, packs this data in 160 64-byte packets and sends the data via WinUSB as single transfer. To keep WinUSB busy, I submit few (i.e. 4) such transfers is 'overlapped' mode, which are then sent to the Arduino by WinUSB sequentially. This adds latency, but pixel response time of the display is slow anyway. The display clients should acquire the named mutex before accessing shared memory. You can compile the code with MinGW, or use the Code::Blocks project.

And the last part is the demo, which is a Python script, loads or draws grayscale PIL image, puts the image bytes (img.tobytes()) into shared memory buffer and switches the active frame (also via shared memory). 

YouTube video of the demo is here: https://youtu.be/mMqvBnYOjEQ

## Notes

As we need Bulk endpoint, Arduino tries to present itself as WCID compliant, Vendor Class USB device (see https://github.com/pbatard/libwdi/wiki/WCID-Devices) to the host, my humble implementation of WICD descriptors. Hopefully this will enable automatic installation of winusb.sys driver under Windows. I wasn't able to test this part much, depends if you played with Zadig before with the same device VID/PID. The required part is to get the 'DeviceInterfaceGUID' value linked to the device, so the host process can find it. 

For various limitations of WinUSB, screen data is packaged in 64 byte USB packets of which first byte is packet number, than 60 bytes of data payload (I am wasting 3 bytes/packet). Each LCD screen line need 40 bytes of data, so in two USB packets I am providing data for 3 screen lines. Or inside the code, I count this in 20 byte 'buffers' and I increase the counter by 3 when I receive USB packet and decrease it by 2 when I shift-out a line.

Touchpanel is read via endpoint 0 with vendor control transfers. Due to WCID, I already had the hooks in place , so I used those. Seems to work. The proper way would be to add one more Interrupt IN endpoint...

The data transfer is paced by the Arduino. When both endpoint banks are full, the USB controller NAKs the packets. 

The host 'driver' is actually simple console application. You can probably build a service around it (or reimplement it as one) but be careful if you run it under different user (i.e. LocalService), you may need to change the name of the shared memory from 'Local\' to 'Global\' (which requires elevated privileges).

The Arduino ISR takes about 15&micro;s to shift-out from the ring buffer and further 25&micro;s to move the data from USB FIFO to the ring buffer (if packet is available). With overhead, this leaves very little time for any "user" code. Probably just enough to read the touchpanel and handle other USB tasks. 

On ProMicro, display plus power control plus touchpanel leave very few pins available. Besides USB, you can probably have only one other communication interface (i.e. HW Serial or I2C or SPI). If you need one of those, remap the configuration to free the pins.

