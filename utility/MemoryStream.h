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
 * @brief Contains the MemoryStream for accessing RAM as a Stream
 */

#include <Arduino.h>

#ifndef _MEMORY_STREAM_H_
#define _MEMORY_STREAM_H_

/**
 * @brief Stream class implementation for reading RAM memory
 */
class MemoryStream : public Stream {
 private:
  const uint8_t* _data;
  uint16_t _length;
  uint16_t _pos;
 public:
  /// Creates a new MemoryStream starting at the address in RAM specified
  MemoryStream(const void* data, uint16_t length);
  virtual int read();
  virtual int peek();
  virtual int available();
  virtual void flush();
  void seek(uint16_t position);
  /// Resets the stream to the beginning
  void reset(void);
  /// Writing is as of now not supported
  size_t write(uint8_t val);
};

#endif