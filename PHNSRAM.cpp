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

/* 
 * Macros to enable, disable or use the SRAM chip functions
 * When enabling, a WAKE is performed by rapidly toggling the hold pin
 */
#define SRAM_EN()       EXSRAM_HOLD_PORT &= ~EXSRAM_HOLD_MASK;
#define SRAM_Disable()  EXSRAM_HOLD_PORT |= EXSRAM_HOLD_MASK;
#define SRAM_Enable()   SRAM_EN(); SRAM_Disable(); SRAM_EN();
#define SRAM_Wait()     while (!(SPSR & (1 << SPIF)));
#define SRAM_Send(b)    SPDR = b; SRAM_Wait();

/* Command codes */
#define SRAM_CMD_STATUS_WRITE 0x1
#define SRAM_CMD_DATA_WRITE   0x2
#define SRAM_CMD_DATA_READ    0x3
#define SRAM_CMD_STATUS_READ  0x5

uint8_t PHN_SRAM::begin() {
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
  SRAM_Enable();
  SRAM_Send(SRAM_CMD_STATUS_WRITE);
  SRAM_Send(64);
  SRAM_Disable();

  // test connection with the device by reading/writing the first bytes
  char data[20];
  uint8_t i;
  uint8_t c = 2;
  uint8_t success = 1;
  // Read data, then write-verify the bit-inverted data twice
  // By doing so, the original data stored is not altered
  readBlock(0, data, sizeof(data));
  do {
    for (i = 0; i < sizeof(data); i++) data[i] ^= 0xFF;
    success &= writeBlockVerify(0, data, sizeof(data));
  } while (--c);

  return success;
}

void PHN_SRAM::readBlock(uint16_t address, char* data, uint16_t length) {
  SRAM_Enable();
  setAddress(SRAM_CMD_DATA_READ, address);
  data--;
  while (length) {
    SPDR = 0xFF;
    data++;
    length--;
    SRAM_Wait();
    *data = SPDR;
  }
  SRAM_Disable();
}

void PHN_SRAM::writeBlock(uint16_t address, const char* data, uint16_t length) {
  SRAM_Enable();
  setAddress(SRAM_CMD_DATA_WRITE, address);
  while (length) {
    SPDR = *data;
    data++;
    length--;
    SRAM_Wait();
  }
  SRAM_Disable();
}

void PHN_SRAM::readSegment(uint16_t index, void* ptr, uint16_t segmentSize) {
  readBlock(index*segmentSize, (char*) ptr, segmentSize);
}

void PHN_SRAM::writeSegment(uint16_t index, const void* ptr, uint16_t segmentSize) {
  writeBlock(index*segmentSize, (const char*) ptr, segmentSize);
}

uint8_t PHN_SRAM::writeBlockVerify(uint16_t address, const char* data, uint16_t length) {
  writeBlock(address, data, length);
  return verifyBlock(address, data, length);
}

uint8_t PHN_SRAM::verifyBlock(uint16_t address, const char* data, uint16_t length) {
  uint8_t success = 1;
  SRAM_Enable();
  setAddress(SRAM_CMD_DATA_READ, address);
  data--;
  while (length && success) {
    SPDR = 0xFF;
    data++;
    length--;
    SRAM_Wait();
    success &= (*data == (char) SPDR);
  }
  SRAM_Disable();
  return success;
}

char PHN_SRAM::read(uint16_t address) {
  char dataByte;
  readBlock(address, &dataByte, 1);
  return dataByte;
}

void PHN_SRAM::write(uint16_t address, char dataByte) {
  writeBlock(address, &dataByte, 1);
}

void PHN_SRAM::setAddress(uint8_t mode, uint16_t address) {
  SRAM_Send(mode);
  SRAM_Send((address >> 8) & 0xFF);
  SRAM_Send(address & 0xFF);
}