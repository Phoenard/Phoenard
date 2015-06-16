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

/**
 * @file
 * @brief Contains the FlashMemoryStream for reading flash memory as a stream of data
 */

#include <Arduino.h>
#include <avr/pgmspace.h>

#ifndef _FLASH_MEM_STREAM_H_
#define _FLASH_MEM_STREAM_H_

/**
 * @brief Reads flash memory as a data stream
 *
 * Is used to allow RAM, SD and FLASH memory to be accessed as a Stream
 */
class FlashMemoryStream : public Stream {
 private:
  const uint8_t *_startAddress;
  uint32_t _pos;
  uint32_t _length;
 public:
  /// Creates a new Flash Memory stream reading from at the address specified
  FlashMemoryStream(const void *startAddress, uint32_t length = 0xFFFFFFFF);

  virtual int read();
  virtual int peek();
  virtual int available();
  virtual void flush();
  /// Seeks the stream to a certain position in memory
  void seek(uint16_t position);
  /// Resets the stream to the beginning
  void reset(void);
  /// Writing to flash memory is not supported - does nothing
  size_t write(uint8_t val);
};

#endif