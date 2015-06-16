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
 * @brief Various utility functions for use all-around
 */

#include <Arduino.h>

#ifndef __PHNUTILS_H__
#define __PHNUTILS_H__

/// Swaps a signed integer around
#define swap(a, b) { int16_t t = a; a = b; b = t; }

/// Gets how much RAM is still available
int getFreeRAM(void);
/// Allocates a new copy of a String on the heap
char* allocateCopy(const char* src);
/// Reverses the byte bit order
uint8_t bit_reverse(uint8_t b);

// Stream utility functions
/// Reads a stream until no further reading is possible
void flushRead(Stream &stream);
/// Waits until a character becomes available to be read
bool waitAvailable(Stream &stream, unsigned long timeoutMS);
/**
 * @brief Flushes one stream to another and vice versa
 *
 * Writes from Stream A to Stream B and vice versa 
 * Put into a While loop for infinite transferring
 * Returns whether any form of communication occurred
 */
bool flushTransfer(Stream &streamA, Stream &streamB);
/**
 * @brief Copies one string to another, counting how many characters are copied
 *
 * The count is EXCLUDING the terminating null-character (is still copied)
 */
int strcpy_count(char* destination, const char* source);

/* Utility classes are included here as they may use PHNUtils.h */
#include "BufferedReadStream.h"
#include "FlashMemoryStream.h"
#include "MemoryStream.h"
#include "DataCopyBuffer.h"

#endif
