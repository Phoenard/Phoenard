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

#include "DataBuffer.h"

DataBuffer::DataBuffer(const void* data, int dataSize) {
  this->data = NULL;
  set(data, dataSize);
}

DataBuffer::~DataBuffer() {
  free(data);
}

void DataBuffer::growToFit(int newDataSize) {
  if (this->dataSize >= newDataSize) return;
  this->dataSize = newDataSize;
  this->data = realloc(this->data, this->dataSize);
}

void DataBuffer::resize(int newDataSize) {
  if (this->dataSize == newDataSize) return;
  this->dataSize = newDataSize;
  this->data = realloc(this->data, this->dataSize);
}

void DataBuffer::set(const void* data, int dataSize) {
  free(this->data);
  this->dataSize = dataSize;
  this->data = malloc(dataSize);
  memcpy(this->data, data, dataSize);
}

void DataBuffer::setTextRaw(const char* text, int textLen) {
  set(text, textLen+1);
}

DataBuffer& DataBuffer::operator=( const DataBuffer& other ) {
  set(other.data, other.dataSize);
  return *this;
}