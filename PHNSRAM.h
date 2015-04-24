/*
The MIT License (MIT)

This file is part of the Phoenard Arduino library
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
*/

/** @file
@brief Contains the PHN_SRAM class for accessing the on-board external 256 KBit (32 Kilobyte) SRAM chip
*/

#ifndef _PHN_SRAM_H_
#define _PHN_SRAM_H_

#include <Arduino.h>
#include "PHNCore.h"

/**
 * @brief Simplistic library for accessing the 32 Kilobyte 23K256 SRAM chip
 *
 * Make use of the global sram variable to make use of this class.
 * Before using, call begin() to set up the SPI and initialize the chip.
 * After that, the read/write functions can be called freely to access the data.
 * Access to address 32768 and beyond wrap around back to 0.
 */
class PHN_SRAM {
 public:
  /// Initializes SPI and sets the chip up for first use
  void begin();

  /// Reads a block of data
  void readBlock(uint16_t address, char* data, uint16_t length);
  /// Writes a block of data
  void writeBlock(uint16_t address, const char* data, uint16_t length);

  /// Reads the byte of data stored at an address specified
  char read(uint16_t address);
  /// Writes a byte of data at an address specified
  void write(uint16_t address, char dataByte);
};

/// Global variable from which the SRAM functions can be accessed
extern PHN_SRAM sram;

#endif