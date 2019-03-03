# Arduino driver for 4-bit controllerless graphics LCD

## Summary

Repository contains code samples for driving 4-bit parallel controllerless graphics LCD (CLGLCD) module with AVR MCU on an Arduino board, using minimal external components and staying within Arduino IDE. 

## 4-bit Controllerless Graphics LCD modules

Controllerless graphics LCD modules are antiques that can be salvaged from old copiers, tape libraries, etc... They commonly are missing, well, the controller chip, the one with the memory. Don't go buying one of these, for Arduino usage, even if you find them on sale. They are usually industrial, have poor viewing angles, generally slow response time, and pain to work-with. There, I said my peace... But if you already have one, their size (i.e. 5.7in) or simplicity can have its uses and beauty. 

I have tested this code with 320x240 STN LCD monochrome module marked as F-51543NFU-LW-ADN / PWB51543C-2-V0, salvaged some time ago from retired tape library, without the controller module (which it appears is based on FPGA and wouldn't be of much use anyway).

The same type of interface (4-bit data) with various signal names is present on many industrial modules based on multiplexed column and common row LCD drivers, like LC79401/LC79431. Or this is what is behind the controller IC. They all have some variations like LCD drive voltage (positive or negative, depending on temperature and size of the module), backlight (LED/CCFL), some logic quirks (i.e. _CL2_ is ignored while _CL1_ is up, etc...), so maybe this code can be adapted to other controllerless modules. Module's datasheet is necessity for the connector pinouts and timing requirements. Some modules may even generate LCD drive voltage internally, and outputting it on a pin so actual _V<sub>0</sub>_ driving voltage can be adjusted.

## Control signal names to recognize these modules
- _FLM_ - First Line Marker, a.k.a. FRAME, VSYNC, etc..
- _CL1_ - Row latch pulse, a.k.a. LOAD, HSYNC, etc..
- _CL2_ - Pixel shift clock, a.k.a. CP, etc..
- _M_ - Alternate signal for LCD drive, a.k.a. BIAS, etc...
- _D0_-_D3_ - 4-bit parallel data signals
- Various pins for GND, power, backlight (i.e. VLED+/-), and LCD drive voltage (i.e. V<sub>EE</sub>, V<sub>0</sub>)

## Driving CLGLCD 

CLGLCD devices are actually quite simple: Shift-in single row (horizontal resolution) of 4-bits/_CL2_ pulse data to the column drivers with falling edge of _CL2_ clock. Then, latch the data to the active row with falling edge of _CL1_ pulse. This also shifts-in the vertical row bit, which is provided by _FLM_ (First Line Marker) signal at start of the frame and then shifted 'down' trough common drivers with falling edge of _CL1_ pulse. There is example schematic in the LC79401 datasheet. That's basically it, you just need to do it fast enough for specified refresh rate and keep _CL1_ pulses evenly spaced... 

Additionally the _M_ signal needs to be toggled, typically on start of every frame, which gives you 1/N duty cycle (specified in datasheet) for the LCD bias. LCD crystals need to be driven by AC voltage (or approximation of it) to prevent them remaining stuck in one orientation (damage to the display, see i.e. https://www.youtube.com/watch?v=ZP0KxZl5N2o). Flipping the _M_ signal at required duty cycle usually is all that is needed and rest is taken care of inside the module itself (i.e. LCD drive voltage is varied in several voltage steps). This can also be achieved with external flip-flip that will toggle the _M_ signal on _CL1_ pulse, when _FLM_ signal is up.

Please see 'clglcd_simple' example, for basic principles of how you can display static image (in flash) on the module.

## Driving CLGLCD with AVR MCU

AVR MCUs don't have required SRAM to hold full frame of graphics data (i.e. 9600 bytes for 320x240 GLCD), nor DMA engine to push the data without CPU intervention. However if you considering adding external RAM, this will probably complicate the setup to the point that more powerfull MCU (i.e. STM32, ESP32) with embedded SRAM and DMA are more cost effective. If you insist, consider 23LC512 type of external SRAM which you might use as kind-of DMA engine in SQI mode.

If we consider some form of procedural generation (i.e. font generator), there just might be enough SRAM and speed on 8-bit AVR MCU. Repository contains character generator code for driving this type of module in text mode, with font stored in flash and text screen buffer in SRAM.

### Text mode

First thing to note is that the pixel clock (_CL2_), according to the i.e. LC79401 datasheet, can actually run up to 6MHz. That is something I use to shift-in the single row data as-fast-as-possible, and then latch the data and move the row selector common bit on timer schedule. 

With character generation, for 320 pixels horizontal resolution, I manage to do this in 20&micro;s (4MHz _CL2_ clock x 80 pulses) which leaves 39.5&micro;s to do other things (typical _CL1_ pulse period for tested module is 59.5&micro;s for 70Hz refresh rate). There is some ISR and "management" overhead, so it is more likely AVR is being busy 45% of available time just with the display. On the positive side, text screen buffer is memory mapped and can be written very quickly.

If you need more available "user" time, you can reduce the refresh rate (my module worked OK down to 50Hz) and/or you can increase AVR clock to i.e. 20MHz, both of which will increase the 952 ticks period between interrupts and give you more time for Your stuff.

I use Output Compare timer pin with period of 4 F<sub>CLK</sub> ticks to toggle _CL2_ pin (4MHz with 50% duty cycle) and synchronously place the data on GPIO pins of single port (i.e. PORTB, connected to _D0_-_D3_ on the module), aligning that data is changed on rising edge and stable on falling edge of the _CL2_ signal. At this rate, assembler is required to use the tricks AVR has up its sleeve.  

The heart of the ISR code is this:
```assembly
  ...
  lpm r24, Z 
  ;----------            (CL2 rising edge)
  out %[data_port], r24
  ld r30, X+
  swap r24;              (CL2 rising edge)
  out %[data_port], r24
  lpm r24, Z
  ;----------            (CL2 rising edge)
  out %[data_port], r24
  ...
```

Which takes exactly 8 F<sub>CLK</sub> ticks to shift-in two 4-bit nibbles in an unrolled loop. The current line of the screen buffer is pointed by X and Z<sub>H</sub> points to the current line of the flash-based font.

One other tricky part is to stop the timer exactly after 80 pulses, which may or may not work with some timers (i.e. driving _CL2_ with Timer4 on 32u4 can be problematic).

For _CL1_ pulse, I use another timer/FastPWM pin with period of 952 ticks (or 119 with /8 prescaller @16MHz) for 59.5&micro;s pulse (240 lines x 59.5&micro;s ~= 70Hz refresh), which also triggers ISR where I shift-in the next row data and setup control lines for the next _CL1_ pulse. The same timer is used to toggle _M_ pin in output compare mode, but only on first _CL1_ pulse of the frame. It might not be strictly necessary but I do like toggling _M_ pin together with CL1 falling edge.

The timing of the signals should resemble something like this:

![Big picture](/timings/big-picture.png?raw=true "Big picture")
![FLM latch](/timings/flm_latch.png?raw=true "FLM Latching and CL1 toggle")
![Data shift](/timings/cl2-data.png?raw=true "Data shifting on CL2 clock")

The code uses font character data (256 characters) stored in flash and screen buffer in SRAM (40bytes x number of lines), one byte for each character. Horizontal size of the font is fixed to 8 pixels, i.e. 40 characters per line. Vertical size of the font is configurable, as long as you can fit whole number of lines in display's vertical resolution. For i.e. 240 lines vertical resolution, supported font sizes would be i.e. 6, 8, 10, 12, 15, 16, etc... The smaller the font, more SRAM is needed to hold the screen buffer (and less flash). For 8x8 fonts, you need 40x30 = 1200 bytes of SRAM and 256x8 bytes of flash. With 8x16 font (IMHO, best looking on 320x240) you need 600 bytes of SRAM and 4KB flash for the font. Code can be modified if You need more than one font stored in flash (starting font address can be changed to be variable instead of compile time constant), but only one font can be active at a time or You will need to implement changing the font base inside the ISR. 

Font data is organized as top vertical line of all characters (256 bytes) first, than second line of all characters, etc... until vertical size of the font. Font data must be aligned on 256-byte boundary in the flash. There is a Python helper script in the "misc" directory which can be used to convert Windows TTF font into C header file (clglcd_font.h), with PROGMEM byte array in correct format. You can find good selection of bitmap fonts at https://int10h.org/oldschool-pc-fonts/ and elsewhere.

### Soft fonts

Another useful feature would be to have some amount of character data in memory instead all of it in flash, and being able to change characters on the fly and/or draw smaller graphics on part of the screen. Depending on the available memory, code supports (I haven't tested with more) up to 64 'soft' characters. Soft characters can be defined as starting character codes (0..n) or as ending codes (255-n..255). Like with the fixed (flash based) fonts, order in memory is first line (1 byte) of all characters, than second line of all characters, etc... up to the vertical size of the font. Data must be aligned on 'number of soft characters' bytes boundary. The bigger the font, more SRAM you need for each 'soft' character. For i.e. 64 8x16 characters you will need 1KB of SRAM, aligned on 64 byte boundary. 

See the demo/example of how to use soft fonts, 64 characters as 128x64 graphics.

Note that using mixed soft and fixed characters, in addition of eating SRAM, slows down shifting of row data by 50% (12 F<sub>CLK</sub> ticks vs 8 ticks for 2x4-bit nibbles), i.e. from 20&micro;s to 30&micro;s per row @16Mhz, leading to about 60% of time spent in the ISR. It is a trade-off. If Arduino is mostly just driving the display, it may be useful.

## Hardware

It is up to you to provide connectors to the display module, power for backlight and LCD drive voltage, starting from i.e. 3x9V batteries for testing, to ICs like MAX749 or equivalent. You should also pull-up or pull-down power control signals and DISPOFF (important to be pulled down and not pulsed during i.e. programming or reset of the board). 

Whatever you do, please make sure that LCD drive voltage is applied after (or simultaneously with) logic supply voltage (i.e. +5V) and DISPOFF pin is brought up only after control signals are in place (i.e. there is only one active bit in common drivers and CL1 is pulsing). Otherwise, you may damage the LCD module.

Module will require minimum of 9 control signals:
- _D0_-_D3_, code needs these to be mapped to same port, Px4-Px7 pins respectively. It will also make rest of the Px0-Px3 unusable as GPIO outputs. You can use alternate peripheral function for the pins (i.e. Timer, UART, etc...). If you use them as inputs, be careful as the pull-up resistor may be toggled at random, unless you globally disable pull-ups (PUD)
- _M_, on a timer (PWM) OC pin,
- _CL1_, on a same timer as _M_, but different OC pin,
- _CL2_, on different timer OC pin, 
- _FLM_, on any digital pin
- _DISPOFF_, on any digital pin.
 
Rest depends on how you will power the module. I like to control the backlight and V<sub>EE</sub> separately.

## Using the driver code

Copy clglcd.h and clglcd.cpp files into you Arduino sketch. 

Copy and **edit** clglcd_config.h to match you hardware configuration, including pinout connections and features enabled (i.e. soft characters, etc...). Pin names are not Arduino pin numbers, rather actual AVR pin names (i.e. B,5 is PB5 pin). You may need to lookup in the board pinout/schematics. Also timer outputs are actual OCnX lines, i.e. 2,B is OC2B which is on i.e. PD3 on Arduino UNO). Examples have working pinout assignments for boards I have tested with. 

Generate clglcd_font.h file for your sketch (see "Misc" directory for python script that can do this)

See examples how to init, start and stop the display and use _screen_ array to place characters on the screen.

Compile and upload the sketch, but before connecting the actual LCD module to the Arduino, I recommend using logic analyzer to check if output of the control pins is what You expect (should be similar to images in "timings" directory) and check LCD drive voltages with multimeter.

This is just a basic driver and character generator. The rest should be your code, i.e. your emulation of serial connected LCD can go in the main loop. Please share.

## Interrupts while display is ON

Display needs to be constantly refreshed, which is done in the ISR. While using the display, disabling of the interrupts for anything more than ~30&micro;s will at best produce visual glitches, and if you are unlucky to disable interrupts for more than 60&micro;s while _FLM_ signal is up, you can cause damage to the LCD module (more than one active bit can be shifted in common drivers, making absolute mess with unpredictable consequences). You can disable the display (_DISPOFF_) before you disable interrupts for longer periods. However, disabling the display every 2 seconds to read a DHT sensor will probably look bad.

Any one-wire (ds18b20, DHTxx), clock-less signal (i.e. ws2812b strips), motors, etc., are likely incompatible with using the same Arduino board, or may require significant effort to integrate (i.e. using USART for one-wire protocol). Pro-Micro clones are so cheap that You can probably dedicate one to the display and talk to it i.e. via I<sup>2</sup>C/USB/UART making it just a LCD controller board. 

## Communication

Communication like hardware serial (up to 115200bps), I<sup>2</sup>C or SPI in master mode should work. In slave mode, it will depend if the master/protocol can tolerate device which is periodically unresponsive for 25-35&micro;s, or if you implement some kind of acknowledgement scheme... 

It also depends on the pins you have leftover after I have taken all the good ones.

USB on 32u4 is fine, as long as you don't query the control endpoint too often (slow ISR code). The CDC driver and its API seem to be quick enough.
