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

#include "PHNDisplayHardware.h"
#include "PHNSettings.h"
#include "PHNDisplayFont.c"

/* Splits a 16-bit argument into two 8-bit bytes in memory */
#define ARG(value)   ((value) & 0xFF), ((value) >> 8)

/* Initial port states */
#define INIT_DDR_MASK   ((1 << TFTLCD_CS_PIN) | (1 << TFTLCD_RS_PIN) | (1 << TFTLCD_WR_PIN) | (1 << TFTLCD_RD_PIN) | (1 << TFTLCD_RESET_PIN))
#define INIT_PORT_MASK  ((1 << TFTLCD_RS_PIN) | (1 << TFTLCD_WR_PIN) | (1 << TFTLCD_RD_PIN) | (1 << TFTLCD_RESET_PIN))

/* Constants for fast reading/writing of data - not to be used for commands */
#define WR_WRITE_A  (INIT_PORT_MASK & ~(1 << TFTLCD_WR_PIN))
#define WR_WRITE_B  (INIT_PORT_MASK |  (1 << TFTLCD_WR_PIN))
#define WR_READ_A  (INIT_PORT_MASK & ~(1 << TFTLCD_RD_PIN))
#define WR_READ_B  (INIT_PORT_MASK |  (1 << TFTLCD_RD_PIN))

/* Specific reading/writing constants for usage by commands */
#define WR_COMMAND_WRITE_A  (WR_WRITE_A & ~(1 << TFTLCD_RS_PIN))
#define WR_COMMAND_WRITE_B  (WR_WRITE_B & ~(1 << TFTLCD_RS_PIN))

/* Stores 8-bit command and 16-bit argument all in one byte array */
const unsigned char LCD_REG_DATA[] = {
  LCD_CMD_START_OSC,          ARG(0x0001),
  LCD_CMD_DRIV_OUT_CTRL,      ARG(0x0100),
  LCD_CMD_DRIV_WAV_CTRL,      ARG(0x0700),
  LCD_CMD_RESIZE_CTRL,        ARG(0x0000),
  LCD_CMD_DISP_CTRL2,         ARG(0x0202),
  LCD_CMD_DISP_CTRL3,         ARG(0x0000),
  LCD_CMD_DISP_CTRL4,         ARG(0x0000),
  LCD_CMD_RGB_DISP_IF_CTRL1,  ARG(0x0000),
  LCD_CMD_FRM_MARKER_POS,     ARG(0x0000),
  LCD_CMD_RGB_DISP_IF_CTRL2,  ARG(0x0000),
  LCD_CMD_POW_CTRL1,          ARG(0x0000),
  LCD_CMD_POW_CTRL2,          ARG(0x0000),
  LCD_CMD_POW_CTRL3,          ARG(0x0000),
  LCD_CMD_POW_CTRL4,          ARG(0x0000),
  LCD_CMD_POW_CTRL1,          ARG(0x17b0),
  LCD_CMD_POW_CTRL2,          ARG(0x0037),
  LCD_CMD_POW_CTRL3,          ARG(0x0138),
  LCD_CMD_POW_CTRL4,          ARG(0x1700),
  LCD_CMD_POW_CTRL7,          ARG(0x000d),
  LCD_CMD_GAMMA_CTRL1,        ARG(0x0001),
  LCD_CMD_GAMMA_CTRL2,        ARG(0x0606),
  LCD_CMD_GAMMA_CTRL3,        ARG(0x0304),
  LCD_CMD_GAMMA_CTRL4,        ARG(0x0103),
  LCD_CMD_GAMMA_CTRL5,        ARG(0x011d),
  LCD_CMD_GAMMA_CTRL6,        ARG(0x0404),
  LCD_CMD_GAMMA_CTRL7,        ARG(0x0404),
  LCD_CMD_GAMMA_CTRL8,        ARG(0x0404),
  LCD_CMD_GAMMA_CTRL9,        ARG(0x0700),
  LCD_CMD_GAMMA_CTRL10,       ARG(0x0a1f),
  LCD_CMD_HOR_START_AD,       ARG(0x0000),
  LCD_CMD_HOR_END_AD,         ARG(0x00ef),
  LCD_CMD_VER_START_AD,       ARG(0x0000),
  LCD_CMD_VER_END_AD,         ARG(0x013f),
  LCD_CMD_GATE_SCAN_CTRL1,    ARG(0x2700),
  LCD_CMD_GATE_SCAN_CTRL2,    ARG(0x0001),
  LCD_CMD_GATE_SCAN_CTRL3,    ARG(0x0000),
  LCD_CMD_PANEL_IF_CTRL1,     ARG(0x0010),
  LCD_CMD_PANEL_IF_CTRL2,     ARG(0x0000),
  LCD_CMD_PANEL_IF_CTRL3,     ARG(0x0003),
  LCD_CMD_PANEL_IF_CTRL4,     ARG(0x0101),
  LCD_CMD_PANEL_IF_CTRL5,     ARG(0x0000),
  LCD_CMD_PANEL_IF_CTRL6,     ARG(0x0000),
  LCD_CMD_DISP_CTRL1,         ARG(0x0133)
};

namespace PHNDisplayHW {

  void init() {
    /* Initialize backlight and data pin to output high */
    TFTLCD_DATADDR = 0xFF;
  /*TFTLCD_BL_DDR  = (1 << TFTLCD_BL_PIN);*/
    DDRL           = (1 << TFTLCD_BL_PIN);
    TFTLCD_BL_PORT = (1 << TFTLCD_BL_PIN);

    /* Initialize LCD port registers */
    DDRK  = INIT_DDR_MASK;
    PORTK = INIT_PORT_MASK;

    /* Reset, wait until LCD is reset (and initialized) */
    TFTLCD_RESET_PORT &= ~(1 << TFTLCD_RESET_PIN);
    delay(2);
    TFTLCD_RESET_PORT |=  (1 << TFTLCD_RESET_PIN);
    delay(32);

    /* Initialize the LCD registers */
    unsigned char i = 0;
    do {
      writeRegister(LCD_REG_DATA[i], *((unsigned int*) (LCD_REG_DATA + i + 1)));
    } while ((i += 3) < sizeof(LCD_REG_DATA));

    /* Fill the screen with BLACK (0x00) */
    PHNDisplay8Bit::drawLine(0, 0, PIXELS, DIR_RIGHT, 0x00);
  }

  void writeData(uint16_t data) {
    /* Write the first byte */
    TFTLCD_WR_PORT = WR_WRITE_A;
    TFTLCD_DATAPORT = data >> 8;
    TFTLCD_WR_PORT = WR_WRITE_B;

    /* Write the second byte */
    TFTLCD_WR_PORT = WR_WRITE_A;
    TFTLCD_DATAPORT = data & 0xFF;
    TFTLCD_WR_PORT = WR_WRITE_B;
  }

  void writeCommand(uint8_t cmd) {
    // Write the LOW byte as 0, only set HIGH byte as requested
    TFTLCD_WR_PORT = WR_COMMAND_WRITE_A;
    TFTLCD_DATAPORT = 0;
    TFTLCD_WR_PORT = WR_COMMAND_WRITE_B;
    TFTLCD_WR_PORT = WR_COMMAND_WRITE_A;
    TFTLCD_DATAPORT = cmd;
    TFTLCD_WR_PORT = WR_COMMAND_WRITE_B;
  }

  void writeRegister(uint8_t cmd, uint16_t arg) {
    writeCommand(cmd);
    writeData(arg);
  }

  uint16_t readData() {
    uint16_t data;

    /* Set to READ mode */
    TFTLCD_DATADDR = 0x00;

    /* Read in both bytes to complete the data */
    TFTLCD_RD_PORT = WR_READ_A;
    delayMicroseconds(10);
    data = TFTLCD_DATAPIN << 8;
    TFTLCD_RD_PORT = WR_READ_B;
    TFTLCD_RD_PORT = WR_READ_A;
    delayMicroseconds(10);
    data |= TFTLCD_DATAPIN;
    TFTLCD_RD_PORT = WR_READ_B;

    // Revert back to WRITE mode, finished
    TFTLCD_DATADDR = 0xFF;
    return data;
  }

  uint16_t readRegister(uint8_t cmd) {
    writeCommand(cmd);
    return readData();
  }

  void setCursor(uint16_t x, uint16_t y, uint8_t direction) {
    /* Setup the CGRAM position to the specified x/y coordinate */
    writeRegister(LCD_CMD_GRAM_VER_AD, (WIDTH - 1) - x);
    writeRegister(LCD_CMD_GRAM_HOR_AD, (HEIGHT - 1) - y);
    writeRegister(LCD_CMD_ENTRY_MOD, 0x1000 | direction);
    writeCommand(LCD_CMD_RW_GRAM);
  }

  void setCursor(uint16_t x, uint16_t y) {
    setCursor(x, y, DIR_RIGHT_WRAP_DOWN);
  }

  void setViewport(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    uint16_t tmp;

    // Transform needed up front
    x1 = (WIDTH - 1) - x1;
    x2 = (WIDTH - 1) - x2;
    y1 = (HEIGHT - 1) - y1;
    y2 = (HEIGHT - 1) - y2;

    // Sort lowest-highest
    if (y2 < y1) {
      tmp = y1;
      y1 = y2;
      y2 = tmp;
    }
    if (x2 < x1) {
      tmp = x1;
      x1 = x2;
      x2 = tmp;
    }

    // Update screen
    writeRegister(LCD_CMD_HOR_START_AD, y1);
    writeRegister(LCD_CMD_HOR_END_AD, y2);
    writeRegister(LCD_CMD_VER_START_AD, x1);
    writeRegister(LCD_CMD_VER_END_AD, x2);
  }

  color_t color565(uint8_t r, uint8_t g, uint8_t b) {
    color_t c;
    c = r >> 3;
    c <<= 6;
    c |= g >> 2;
    c <<= 5;
    c |= b >> 3;

    return c;
  }

  color_t colorAverage(color_t colorA, color_t colorB) {
    int r = (int) color565Red(colorA) + (int) color565Red(colorB);
    int g = (int) color565Green(colorA) + (int) color565Green(colorB);
    int b = (int) color565Blue(colorA) + (int) color565Blue(colorB);
    return color565(r >> 1, g >> 1, b >> 1);
  }

  uint8_t color565Red(color_t color) {
    return (color & 0x001F) << 3;
  }

  uint8_t color565Green(color_t color) {
    return (color & 0x07E0) >> 3;
  }

  uint8_t color565Blue(color_t color) {
    return (color & 0xF800) >> 8;
  }

  static void setTouchPins(int lowPin, int highPin, int HiZPin1, int HiZPin2) {
    pinMode(lowPin, OUTPUT);
    digitalWrite(lowPin, LOW);
    pinMode(highPin, OUTPUT);
    digitalWrite(highPin, HIGH);
    pinMode(HiZPin1, INPUT);
    digitalWrite(HiZPin1, LOW);
    pinMode(HiZPin2, INPUT);
    digitalWrite(HiZPin2, LOW);
    delay(1);
  }

  void readTouch(uint16_t *analogX, uint16_t *analogY, uint16_t *analogZ1, uint16_t *analogZ2) {
    // First turn the LCD off
    TFTLCD_CS_PORT |= (1 << TFTLCD_CS_PIN);

    // First probe the pressure readout to check if there is a valid press
    // If there is not, skip the x/y readout and indicate no press
    setTouchPins(TFTLCD_PIN_XP, TFTLCD_PIN_YM, TFTLCD_PIN_YP, TFTLCD_PIN_XM);
    if (analogRead(TFTLCD_PIN_XM)) {
      // ========================================
      // Set X+ to Hi-Z
      // Set X- to Hi-Z
      // Set Y+ to HIGH
      // Set Y- to LOW
      setTouchPins(TFTLCD_PIN_YM, TFTLCD_PIN_YP, TFTLCD_PIN_XM, TFTLCD_PIN_XP);
      *analogX = analogRead(TFTLCD_PIN_XM);
      // ========================================

      // ========================================
      // Set Y+ to Hi-Z
      // Set Y- to Hi-Z   
      // Set X+ to HIGH  
      // Set X- to LOW
      setTouchPins(TFTLCD_PIN_XM, TFTLCD_PIN_XP, TFTLCD_PIN_YM, TFTLCD_PIN_YP);
      *analogY = analogRead(TFTLCD_PIN_YP);
      // ========================================

      // ========================================
      // Set X+ to LOW
      // Set Y- to HIGH
      // Set Y+ to Hi-Z
      // Set X- to Hi-Z
      setTouchPins(TFTLCD_PIN_XP, TFTLCD_PIN_YM, TFTLCD_PIN_YP, TFTLCD_PIN_XM);
      *analogZ1 = analogRead(TFTLCD_PIN_XM); 
      *analogZ2 = analogRead(TFTLCD_PIN_YP);
      // ========================================
    } else {
      // Set result to a default, no-press state
      *analogX = 0;
      *analogY = 0;
      *analogZ1 = 0;
      *analogZ2 = 1023;
    }

    // Restore pins to outputs in the default HIGH state
    pinMode(TFTLCD_PIN_XM, OUTPUT);
    pinMode(TFTLCD_PIN_YP, OUTPUT);
    digitalWrite(TFTLCD_PIN_XM, HIGH);
    digitalWrite(TFTLCD_PIN_YP, HIGH);

    // All done, turn the chip back on
    TFTLCD_CS_PORT &= ~(1 << TFTLCD_CS_PIN);
  }
  
  void readTouch(uint16_t *touch_x, uint16_t *touch_y, float *pressure) {
    uint16_t analogX, analogY, analogZ1, analogZ2;
    int hor_a, hor_b, ver_a, ver_b;

    // First read the touch raw input
    PHNDisplayHW::readTouch(&analogX, &analogY, &analogZ1, &analogZ2);

    // Proceed to use the calibration found in EEPROM to transform the analog X/Y
    PHN_Settings settings;
    PHN_Settings_Load(settings);
    PHN_Settings_ReadCali(settings, &hor_a, &hor_b, &ver_a, &ver_b);
    *touch_x = map(analogX, hor_a, hor_b, 0, WIDTH - 1);
    *touch_y = map(analogY, ver_a, ver_b, 0, HEIGHT - 1);

    // If touch x/y are out of range (negative turns into a high value!) assume not pressed
    // Use the width that includes the dead space (slider) to the right
    if (*touch_x >= WIDTH_SLIDER || *touch_y >= HEIGHT) {
      *pressure = 0.0F;
      return;
    }

    // Transforms the analog read into a raw pressure estimate
    *pressure = (float) analogZ1 / (float) analogZ2;
    *pressure *= analogY;
    
    // Pressure has odd top and bottom screen borders
    // These values were found using weight experiments.
    if (analogY < 400) {
      *pressure += *pressure * (0.004 * (400 - analogY));
    } else if (analogY > 700) {
      *pressure += *pressure * (0.004 * (analogY - 700));
    }
  }
}

namespace PHNDisplay8Bit {

  void writePixel(uint8_t color) {  
    /* Spam the write instruction 2x */
    /* Use NOPs to ensure 2 clock cycles between toggling */
    TFTLCD_WR_PORT = WR_WRITE_A;
    TFTLCD_DATAPORT = color;
    TFTLCD_WR_PORT = WR_WRITE_B;
    TFTLCD_WR_PORT = WR_WRITE_A;
    asm volatile ("nop\n");
    TFTLCD_WR_PORT = WR_WRITE_B;
  }

  void writePixels(uint8_t color, uint32_t length) {
    /* Write the data to the data port */
    TFTLCD_DATAPORT = color;

    /* For each pixel, perform 2 8-bit writes */
    length <<= 1;
    while (length) {
      /* Perform length reduction inside the toggle to introduce a pin toggle delay */
      TFTLCD_WR_PORT = WR_WRITE_A;
      length--;
      TFTLCD_WR_PORT = WR_WRITE_B;
    }
  }

  void writePixelLines(uint8_t color, uint8_t lines) {
    writePixels(color, (uint32_t) lines * PHNDisplayHW::WIDTH);
  }

  void drawLine(uint16_t x, uint16_t y, uint32_t length, uint8_t direction, uint8_t color) {
    PHNDisplayHW::setCursor(x, y, direction);
    writePixels(color, length);
  }

  void drawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t color) {
    drawLine(x, y, width, DIR_RIGHT, color);
    drawLine(x, y + height - 1, width, DIR_RIGHT, color);
    drawLine(x, y, height, DIR_DOWN, color);
    drawLine(x + width - 1, y, height, DIR_DOWN, color);
  }

  void fillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t color) {
    uint16_t dy;
    for (dy = 0; dy < height; dy++) {
      drawLine(x, y + dy, width, DIR_RIGHT, color);
    }
  }

  void fill(uint8_t color) {
    PHNDisplayHW::setCursor(0, 0);
    writePixels(color, PHNDisplayHW::PIXELS);
  }

  /*
   * Slightly less efficient in drawing, but prevents duplicate code
   * For line pixel drawing, an 8-bit verification check is performed using 8-bit mode there too
   * This likely will not get optimized out, though.
   */

  void writeString(uint16_t x, uint16_t y, uint8_t scale, const char* text, uint8_t color0, uint8_t color1) {
    PHNDisplay16Bit::writeString(x, y, scale, text, COLOR8TO16(color0), COLOR8TO16(color1));
  }

  void writeChar(uint16_t x, uint16_t y, uint8_t scale, char c, uint8_t color0, uint8_t color1) {
    PHNDisplay16Bit::writeChar(x, y, scale, c, COLOR8TO16(color0), COLOR8TO16(color1));
  }

  void writeFont_1bit(uint16_t x, uint16_t y, uint8_t scale, const uint8_t* data, uint8_t color0, uint8_t color1) {
    PHNDisplay16Bit::writeFont_1bit(x, y, scale, data, COLOR8TO16(color0), COLOR8TO16(color1));
  }

  void writeImage_1bit(uint16_t x, uint16_t y, uint8_t width, uint8_t height, uint8_t scale, const uint8_t* data, uint8_t direction, uint8_t color0, uint8_t color1) {
    PHNDisplay16Bit::writeImage_1bit(x, y, width, height, scale, data, direction, COLOR8TO16(color0), COLOR8TO16(color1));
  }
}

namespace PHNDisplay16Bit {

  void writePixel(uint16_t color) {
    PHNDisplayHW::writeData(color);
  }

  void writePixels(uint16_t color, uint32_t length) {
    uint8_t data_a = (color >> 8);
    uint8_t data_b = (color & 0xFF);
    if (data_a == data_b) {
      /* If the first and second byte of the 16-bit data are equal, 
       * use the 8-bit write function instead */
      PHNDisplay8Bit::writePixels(data_a, length);
    } else {
      /* First and second byte not equal - write each byte alternating */
      while (length) {
        /* Write the first byte, subtract length within data write to add delay */
        TFTLCD_WR_PORT = WR_WRITE_A;
        TFTLCD_DATAPORT = data_a;
        length--;
        TFTLCD_WR_PORT = WR_WRITE_B;
        /* Write the second byte, fill cycles with NOPs to add delay */
        TFTLCD_WR_PORT = WR_WRITE_A;
        TFTLCD_DATAPORT = data_b;
        asm volatile ("nop\n");
        TFTLCD_WR_PORT = WR_WRITE_B;
      }
    }
  }

  void drawLine(uint16_t x, uint16_t y, uint32_t length, uint8_t direction, uint16_t color) {
    /* Move cursor to the right position */
    PHNDisplayHW::setCursor(x, y, direction);
    /* Write out the pixels */
    writePixels(color, length);
  }
  
  void fill(uint16_t color) {
    PHNDisplayHW::setCursor(0, 0);
    writePixels(color, PHNDisplayHW::PIXELS);
  }

  void writeString(uint16_t x, uint16_t y, uint8_t scale, const char* text, uint16_t color0, uint16_t color1) {
    uint16_t c_x = x;
    uint16_t c_y = y;
    while (*text) {
      if (*text == '\n') {
        c_x = x;
        c_y += 8*scale;
      } else {
        writeChar(c_x, c_y, scale, *text, color0, color1);
        c_x += 6*scale;
      }
      text++;
    }
  }

  void writeChar(uint16_t x, uint16_t y, uint8_t scale, char c, uint16_t color0, uint16_t color1) {
    writeFont_1bit(x, y, scale, font_5x7+(c*5), color0, color1);
  }

  void writeFont_1bit(uint16_t x, uint16_t y, uint8_t scale, const uint8_t* data, uint16_t color0, uint16_t color1) {
    // Read character data into memory
    uint8_t c[5];
    memcpy_P(c, (void*) data, 5);
    // Use standard 1-bit drawing function and draw the character pixel data
    writeImage_1bit(x, y, 8, 5, scale, c, DIR_DOWN, color0, color1);
  }

  void writeImage_1bit(uint16_t x, uint16_t y, uint8_t width, uint8_t height, uint8_t scale, const uint8_t* data, uint8_t direction, uint16_t color0, uint16_t color1) {
    uint8_t pix_dat, dy, si, dx, d;
    for (dy = 0; dy < height; dy++) {
      for (si = 0; si < scale; si++) {
        PHNDisplayHW::setCursor(x, y, direction);
        if (direction == DIR_DOWN) {
          x++;
        } else {
          y++;
        }

        const uint8_t* data_line = data;
        for (dx = 0; dx < width; dx++) {
          /* Refresh pixel data every 8 pixels */
          if ((dx & 0x7) == 0) pix_dat = *data_line++;
          /* Draw SCALE pixels for each pixel in a line */
          writePixels((pix_dat & 0x1) ? color1 : color0, scale);
          /* Next pixel data bit */
          pix_dat >>= 1;
        }
      }
      data += width/8;
    }
  }
}