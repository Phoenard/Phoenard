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
 * 
 * Constants use the following naming scheme:
 * - All constants start with the name of the component controlled
 * - If multiple functions exist, the function is appended
 * - Constant type is then declared:
 * |- PIN: Digital (Arduino) Pin
 * |- DDR: Direction port register
 * |- PORT: Output port register
 * |- IN: Input port read register
 * |- MASK: Mask to use with the port
 * 
 * For example: VS1053_RESET_PIN
 */

#include <inttypes.h>
 
#ifndef _PHN_CORE_
#define _PHN_CORE_

/* SELECT button */
static const uint8_t SELECT_PIN = 38;
#define SELECT_DDR    DDRD
#define SELECT_PORT   PORTD
#define SELECT_IN     PIND
#define SELECT_MASK   _BV(PD7)

/* SPI SS/MOSI/MISO/SCK registers */
#define SPI_DDR        DDRB
#define SPI_PORT       PORTB
#define SPI_SS_MASK    _BV(PB0)
#define SPI_MOSI_MASK  _BV(PB1)
#define SPI_MISO_MASK  _BV(PB2)
#define SPI_SCK_MASK   _BV(PB3)
#define SPI_MASK       (SPI_SS_MASK | SPI_MOSI_MASK | SPI_MISO_MASK | SPI_SCK_MASK)

/*
 * The initialization states of the SPI PORT/DDR
 * SS_PIN   = OUTPUT, HIGH
 * SCK_PIN  = OUTPUT, LOW
 * MOSI_PIN = OUTPUT, LOW
 * MISO_PIN = INPUT, LOW
 */
#define SPI_INIT_DDR   ((1 * SPI_SS_MASK) | (1 * SPI_MOSI_MASK) | (1 * SPI_MISO_MASK) | (0 * SPI_SCK_MASK))
#define SPI_INIT_PORT  ((1 * SPI_SS_MASK) | (0 * SPI_MOSI_MASK) | (0 * SPI_MISO_MASK) | (0 * SPI_SCK_MASK))

/* Micro-SD chip select (CS) */
static const uint8_t SD_CS_PIN = 10;
#define SD_CS_PORT  PORTB
#define SD_CS_DDR   DDRB
#define SD_CS_MASK  _BV(PB4)

/* External RAM HOLD (CS) */
#define EXSRAM_HOLD_PORT  PORTK
#define EXSRAM_HOLD_DDR   DDRK
#define EXSRAM_HOLD_MASK  _BV(PK0)

/* SIM908 */
static const uint8_t SIM_STATUS_PIN = 39;
static const uint8_t SIM_PWRKEY_PIN = 40;
static const uint8_t SIM_DTRS_PIN = 41;
static const uint8_t SIM_RI_PIN = 3;

/* Bluetooth */
static const uint8_t BLUETOOTH_RESET_PIN = 47;
static const uint8_t BLUETOOTH_KEY_PIN = 12;

/* WiFi */
static const uint8_t WIFI_PWR_PIN = 49;

/* VS1053 Audio Decoder */
static const uint8_t VS1053_CARDCS_PIN = SD_CS_PIN;
static const uint8_t VS1053_PWR_PIN = 43;
static const uint8_t VS1053_CS_PIN = 45;
static const uint8_t VS1053_RESET_PIN = 46;
static const uint8_t VS1053_DCS_PIN = 8;
static const uint8_t VS1053_DREQ_PIN = 2;
static const uint8_t VS1053_GPIO_PIN = 11;
static const uint8_t VS1053_IRX_PIN = 48;
#define VS1053_IRX_PORT  PORTL
#define VS1053_IRX_MASK  _BV(PL1)

/* LCD Data control pins */
static const uint8_t TFTLCD_BL_PIN = 44;
static const uint8_t TFTLCD_CS_PIN = 69;
static const uint8_t TFTLCD_RS_PIN = 68;
static const uint8_t TFTLCD_WR_PIN = 67;
static const uint8_t TFTLCD_RD_PIN = 66;
static const uint8_t TFTLCD_RESET_PIN = 65;

/* LCD Touchscreen pins */
static const uint8_t TFTLCD_YP_PIN = 67;
static const uint8_t TFTLCD_XM_PIN = 68;
static const uint8_t TFTLCD_YM_PIN = 30;
static const uint8_t TFTLCD_XP_PIN = 31;

/* LCD Data control registers */
#define TFTLCD_BL_PORT     PORTL
#define TFTLCD_BL_DDR      DDRL
#define TFTLCD_BL_MASK     _BV(PL5)

#define TFTLCD_RESET_PORT  PORTK
#define TFTLCD_RESET_MASK  _BV(PK3)
#define TFTLCD_RD_PORT     PORTK
#define TFTLCD_RD_MASK     _BV(PK4)
#define TFTLCD_WR_PORT     PORTK
#define TFTLCD_WR_MASK     _BV(PK5)
#define TFTLCD_RS_PORT     PORTK
#define TFTLCD_RS_MASK     _BV(PK6)
#define TFTLCD_CS_PORT     PORTK
#define TFTLCD_CS_MASK     _BV(PK7)

/* LCD Data registers */
#define TFTLCD_DATA_DDR    DDRC
#define TFTLCD_DATA_PORT   PORTC
#define TFTLCD_DATA_IN     PINC

#endif