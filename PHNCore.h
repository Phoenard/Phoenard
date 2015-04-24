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
 * @brief Pinout information for the Phoenard hardware components
 */

#ifndef _PHN_CORE_
#define _PHN_CORE_

#include <Arduino.h>

// Select button
static const uint8_t SELECT_PIN = 38;

/* Masks for each individual SPI pin */
#define SS_MASK 0x01
#define MOSI_MASK 0x02
#define MISO_MASK 0x04
#define SCK_MASK 0x08

/* Masks and PORT/DDR buses for the chip select pin */
#define SD_CS_PORT  PORTB
#define SD_CS_DDR   DDRB
#define SD_CS_MASK  0x10
static const uint8_t SD_CS_PIN = 10;

/* The PORT and DDR buses for SPI, and the mask to access them */
#define SPI_DDR DDRB
#define SPI_PORT PORTB
#define SPI_MASK (SS_MASK | MOSI_MASK | MISO_MASK | SCK_MASK)

/*
 * The initialization states of the SPI PORT/DDR
 * 0x01 SS_PIN = OUTPUT, HIGH
 * 0x02 SCK_PIN = OUTPUT, LOW
 * 0x04 MOSI_PIN = OUTPUT, LOW
 * 0x08 MISO_PIN = INPUT, LOW
 */
#define SPI_INIT_DDR  ((1 * SS_MASK) | (1 * MOSI_MASK) | (1 * MISO_MASK) | (0 * SCK_MASK))
#define SPI_INIT_PORT ((1 * SS_MASK) | (0 * MOSI_MASK) | (0 * MISO_MASK) | (0 * SCK_MASK))

// External RAM HOLD (CS) pin and port
#define EXSRAM_HOLD_PORT  PORTK
#define EXSRAM_HOLD_DDR   DDRK
#define EXSRAM_HOLD_PIN   0

// SIM908 pins
static const uint8_t SIM_PIN_STATUS = 39;
static const uint8_t SIM_PIN_PWRKEY = 40;
static const uint8_t SIM_PIN_DTRS = 41;
static const uint8_t SIM_PIN_RI = 3;

// Bluetooth pins
static const uint8_t BLUETOOTH_PIN_RESET = 47;
static const uint8_t BLUETOOTH_PIN_KEY = 12;

// WiFi pins
static const uint8_t WIFI_PIN_PWR = 49;

// Audio decoder pins for VS1053 chip
static const uint8_t VS1053_PIN_CARDCS = SD_CS_PIN;
static const uint8_t VS1053_PIN_PWR = 43;
static const uint8_t VS1053_PIN_CS = 45;
static const uint8_t VS1053_PIN_RESET = 46;
static const uint8_t VS1053_PIN_DCS = 8;
static const uint8_t VS1053_PIN_DREQ = 2;
static const uint8_t VS1053_PIN_GPIO = 11;
static const uint8_t VS1053_PIN_IRX = 48;

// Audio decoder MIDI TX port/pin
#define VS1053_IRX_PORT  PORTL
#define VS1053_IRX_PIN   1

// LCD pins
static const uint8_t TFTLCD_PIN_BL = 44;
static const uint8_t TFTLCD_PIN_CS = 69;
static const uint8_t TFTLCD_PIN_RS = 68;
static const uint8_t TFTLCD_PIN_WR = 67;
static const uint8_t TFTLCD_PIN_RD = 66;
static const uint8_t TFTLCD_PIN_RESET = 65;

// Touch pins
static const uint8_t TFTLCD_PIN_YP = 67;
static const uint8_t TFTLCD_PIN_XM = 68;
static const uint8_t TFTLCD_PIN_YM = 30;
static const uint8_t TFTLCD_PIN_XP = 31;

// Port + Mask constants for all LCD pins declared above
#define TFTLCD_BL_PORT     PORTL
#define TFTLCD_BL_PIN      5
#define TFTLCD_CS_PORT     PORTK
#define TFTLCD_CS_PIN      7
#define TFTLCD_RS_PORT     PORTK
#define TFTLCD_RS_PIN      6
#define TFTLCD_WR_PORT     PORTK
#define TFTLCD_WR_PIN      5
#define TFTLCD_RD_PORT     PORTK
#define TFTLCD_RD_PIN      4
#define TFTLCD_RESET_PORT  PORTK
#define TFTLCD_RESET_PIN   3

// LCD data setting/getting
#define TFTLCD_DATAPORT    PORTC
#define TFTLCD_DATAPIN     PINC
#define TFTLCD_DATADDR     DDRC

#endif