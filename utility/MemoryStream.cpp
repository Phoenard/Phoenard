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

#include "MemoryStream.h"

MemoryStream::MemoryStream(const void* data) {
  _data = (uint8_t*) data;
  _length = 0xFFFF;
  _pos = 0;
}

MemoryStream::MemoryStream(const void* data, uint16_t length) {
  _data = (uint8_t*) data;
  _length = length;
  _pos = 0;
}

int MemoryStream::peek() {
  return _data[_pos];
}

int MemoryStream::read() {
  if (available()) {
    return _data[_pos++];
  } else {
    return -1;
  }
}

int MemoryStream::available() {
  return _length - _pos;
}

void MemoryStream::flush() {
  // Nothing is done here
}

void MemoryStream::seek(uint16_t position) {
  _pos = position;
}

void MemoryStream::reset() {
  seek(0);
}
  
size_t MemoryStream::write(uint8_t val) {
  return 0;
}