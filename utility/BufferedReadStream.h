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
 * @brief Contains the BufferedReadStream class to read data with a buffer
 */

#include <Arduino.h>

#ifndef _BUFF_READ_STREAM_H_
#define _BUFF_READ_STREAM_H_

/**
 * @brief Buffered stream implementation for reading another stream with a buffer
 *
 * Reads in multiple bytes at once into the buffer, allowing faster reading if the
 * original stream byte-by-byte reading function is too slow.
 */
class BufferedReadStream : public Stream {
private:
  Stream *_baseStream;
  unsigned char *_buffer;
  int _bufferLen;
  int _bufferPos;
  void refreshBuffer();
public:
  /// Constructs a new buffered read stream reading from the baseStream
  BufferedReadStream(Stream* baseStream, int BufferSize);
  ~BufferedReadStream();
  virtual int read();
  virtual int peek();
  virtual int available();
  virtual void flush();
  size_t write(uint8_t val);
};

#endif