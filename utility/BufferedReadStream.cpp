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

#include "BufferedReadStream.h"

BufferedReadStream::BufferedReadStream(Stream *baseStream, uint16_t bufferSize) {
  _baseStream = baseStream;
  _bufferLen = bufferSize;
  _bufferPos = bufferSize;
  _buffer = new unsigned char[bufferSize];
}

BufferedReadStream::~BufferedReadStream() {
  delete[] _buffer;
}

inline void BufferedReadStream::refreshBuffer() {
  if (_bufferPos == _bufferLen) {
    _bufferPos = 0;
    _baseStream->readBytes((char*) _buffer, min(_bufferLen, _baseStream->available()));
  }
}

int BufferedReadStream::peek() {
  refreshBuffer();
  return _buffer[_bufferPos];
}

int BufferedReadStream::read() {
  refreshBuffer();
  if (_bufferLen) {
    return _buffer[_bufferPos++];
  } else {
    return -1;
  }
}

int BufferedReadStream::available() {
  return _bufferLen - _bufferPos + _baseStream->available();
}

void BufferedReadStream::flush() {
  _baseStream->flush();
}

size_t BufferedReadStream::write(uint8_t val) {
  return _baseStream->write(val);
}