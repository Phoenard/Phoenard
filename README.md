# Phoenard Arduino Library

The Phoenard library is a software package written for controlling key features of the Phoenard.
For detailed information and tutorials, [you can visit our website here.](http://phoenard.com)

This library features several key components that were lacking in existing libraries.
Please note that the hardware pinout is optimized to work with the Phoenard hardware and
is built to run on the ATMEGA2560 chip. Although porting the software to a different platform
is quite possible, it will require a little effort and understanding of the platform.

## Library features

The following components are included in this library:

* Phoenard hardware pinout (PHNCore.h)
* TFTLCD ILI9325/ILI9328 low-level hardware control
  * Speed-optimized data writing functions
  * Screen register/color/protocol constants
  * Size-optimized screen initialization routine
  * Basic speed/size-optimized drawing functions
* Display library
  * Shape/font drawing routines
  * Image drawing functions (.BMP/.LCD formats)
    * Draw 1/2/4/8/16/24-bit images with colormap/transform support
    * Stream-based data reading (supports data from any stream)
    * Flash/RAM stream reading wrappers available
    * Image container class for storing image information
  * Touch screen readout
    * Calibration data read from EEPROM
  * Widgets
    * Event loop system with update/draw routines
    * Widget classes can be extended/self-implemented
    * Various widget properties and utilities
    * Readout widgets: Bargraph, Gauge, Label, LineGraph
    * Interactive widgets: Button, ButtonGrid, Scrollbar, TextBox
* EEPROM Settings
  * Functions to request the bootloader to load a new sketch
  * Functions to read/write touchscreen calibration data
  * Read the FLASH size of the currently running sketch
* SIM908 ATCOMMAND library
  * Performing AT-Commands
  * Receiving/sending messages
  * Receiving/making calls
  * Reading inbox messages
  * Reading/writing contacts
  * Power control/battery level/signal strength
* Basic MIDI control library
* Basic 23K256 external SRAM library
* Minimal (size) Micro-SD library (read/write FAT16/FAT32 filesystems)

## Examples

Plenty of examples are available for both beginning and intermediate developers.
Software to calibrate the screen, the sketch list and much more are available.

## Optimizations

Most of this library is written with user-friendliness being the primary goal.
Some parts of this software library are also used by the size-optimized bootloader.
Because of this, a few parts of this library are a lot more rough to use than others.
These cover mainly the low-level display library and the minimal micro-sd library.

## Risks

The SD-Minimal implementation is heavily optimized for size. It requires thorough
understanding of its operations, since most safety checks are tossed out in favor
of reducing code size to the minimal. Please read the software documentation thoroughly
before making use of it, as you may end up corrupting your Micro-SD card.

Be careful with using SIM908 functionality. Do not create programs such as auto-dialers
and spam machines, since this may be illegal (local law), cause extra charges (telecom)
or otherwise get you in trouble. This can happen accidentally as well, so when writing
software, always make sure your code won't loop rapidly (use delays) or provide a kill-switch.

## Links

* [View our website](http://phoenard.com)
* [GitHub repository](https://github.com/Phoenard/Phoenard)
* [View Doxygen documentation](http://docs.phoenard.com)
* [Download Doxygen documentation (zip-file)](http://builds.phoenard.com/phoenard_docs_doxygen.zip)

## License

The MIT License (MIT)

Copyright (c) 2014 Phoenard

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.