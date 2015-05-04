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

#include "PHNSRAM.h"

/* Global instance */
PHN_SRAM sram;

/* Macro to send a byte to SPI */
#define SRAM_Send(b)  SPDR = b; while (!(SPSR & (1 << SPIF)));

/* Command codes */
#define SRAM_CMD_STATUS_WRITE 0x1
#define SRAM_CMD_DATA_WRITE   0x2
#define SRAM_CMD_DATA_READ    0x3
#define SRAM_CMD_STATUS_READ  0x5

void PHN_SRAM::begin() {
  // Initialize SPI I/O
  SPI_DDR = (SPI_DDR & ~SPI_MASK) | SPI_INIT_DDR;
  SPI_PORT = (SPI_PORT & ~SPI_MASK) | SPI_INIT_PORT;
  // Initialize SPI registers
  SPCR |= _BV(MSTR) | _BV(SPE);

  // Set External RAM HOLD pin to output
  EXSRAM_HOLD_DDR |= EXSRAM_HOLD_MASK;

  // Set up status register to sequential mode
  // Both single-byte and block write functions use sequential mode
  // Page mode is never used and appears to be pointless
  EXSRAM_HOLD_PORT &= ~EXSRAM_HOLD_MASK;
  SRAM_Send(SRAM_CMD_STATUS_WRITE);
  SRAM_Send(64);
  EXSRAM_HOLD_PORT |= EXSRAM_HOLD_MASK;
}

void PHN_SRAM::readBlock(uint16_t address, char* data, uint16_t length) {
  EXSRAM_HOLD_PORT &= ~EXSRAM_HOLD_MASK;
  SRAM_Send(SRAM_CMD_DATA_READ);
  SRAM_Send((address >> 8) & 0xFF);
  SRAM_Send(address & 0xFF);
  while (length--) {
    SRAM_Send(0xFF);
    *(data++) = SPDR;
  }
  EXSRAM_HOLD_PORT |= EXSRAM_HOLD_MASK;
}

void PHN_SRAM::writeBlock(uint16_t address, const char* data, uint16_t length) {
  EXSRAM_HOLD_PORT &= ~EXSRAM_HOLD_MASK;
  SRAM_Send(SRAM_CMD_DATA_WRITE);
  SRAM_Send((address >> 8) & 0xFF);
  SRAM_Send(address & 0xFF);
  while (length--) {
    SRAM_Send(*(data++));
  }
  EXSRAM_HOLD_PORT |= EXSRAM_HOLD_MASK;
}

char PHN_SRAM::read(uint16_t address) {
  char dataByte;
  readBlock(address, &dataByte, 1);
  return dataByte;
}

void PHN_SRAM::write(uint16_t address, char dataByte) {
  writeBlock(address, &dataByte, 1);
}