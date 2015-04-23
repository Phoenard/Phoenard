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

/** @addtogroup Display
 *  @{
 */

/**@file
 * @brief Low-level utility functions for working with the display
 * 
 * If maximum speed or minimal code size is required, you can make use of
 * the global methods contained in this file. It introduces 8-bit and 16-bit
 * drawing functions to push out as much data as possible with minimal complexity.
 * It also adds a minimal touch input reading function and drawing basic shapes.
 * 
 * This low-level class is intended for use with the ILI9328/ILI9325 TFT-LCD screen.
 * See also: @link PHN_Display display @endlink - object-oriented wrapper with more advanced logic
 * 
 * Usage
 * -----
 * To draw pixels to the screen, the first step is to move the cursor to the start
 * location. While doing so, you can also specify the direction to move to as you draw.
 * Use the DIR_ constants to specify the direction, which includes some 'wrap' constants.
 * The wrap mode specified what way to 'wrap-around' the screen. When you draw pixels and
 * it reaches the border of the screen region, it can either move one line 'down' or 'up'.
 * Down and up are relative to what direction you are drawing.
 * 
 * In addition to specifying the start location, you can also set a 'viewport'. This
 * selects a rectangular region on the screen and keeps the cursor within this area.
 * Any draw calls outside this region are ignored, and this border is also used for the
 * wrap-around logic. If you need to draw complex information such as an image in a
 * rectangular area, viewports are the way to go.
 * 
 * Difference 8-bit and 16-bit color drawing
 * -----------------------------------------
 * 16-bit (565) color is the default drawing method and pushes out two bytes for each color.
 * It requires bit-shifting to write it out, plus it needs two port writes. For example,
 * the color 0x1234 will first write out 0x34 and then 0x12.
 * 
 * If you, however, make use of the 8-bit drawing functions the port is set only once
 * and the same data byte is written out twice. For example, the 16-bit color 0x5E5E is
 * a valid 8-bit color and it will write out 0x5E two times. This does not require
 * bit-shifting and only sets the port once, making it great for filling many pixels
 * at once.
 * 
 * Both 16-bit and 8-bit color constants are available for use.
 * 
 * Important notes
 * ---------------
 * The use of the init() function is not required when running on the Phoenard bootloader.
 * The bootloader already initializes the display for you, calling it again is redundant.
 */

#include <Arduino.h>
#include "PHNCore.h"

#ifndef PHN_DISPLAY_HARDWARE_H_
#define PHN_DISPLAY_HARDWARE_H_

/// Definition of the 16-bit 565 color type
typedef uint16_t color_t;

/**@name LCD Hardware Commands
 * @brief All commands that can be written out to the display
 */
//@{
#define LCD_CMD_START_OSC          0x00
#define LCD_CMD_DRIV_OUT_CTRL      0x01
#define LCD_CMD_DRIV_WAV_CTRL      0x02
#define LCD_CMD_ENTRY_MOD          0x03
#define LCD_CMD_RESIZE_CTRL        0x04
#define LCD_CMD_DISP_CTRL1         0x07
#define LCD_CMD_DISP_CTRL2         0x08
#define LCD_CMD_DISP_CTRL3         0x09
#define LCD_CMD_DISP_CTRL4         0x0A
#define LCD_CMD_RGB_DISP_IF_CTRL1  0x0C
#define LCD_CMD_FRM_MARKER_POS     0x0D
#define LCD_CMD_RGB_DISP_IF_CTRL2  0x0F
#define LCD_CMD_POW_CTRL1          0x10
#define LCD_CMD_POW_CTRL2          0x11
#define LCD_CMD_POW_CTRL3          0x12
#define LCD_CMD_POW_CTRL4          0x13
#define LCD_CMD_GRAM_HOR_AD        0x20
#define LCD_CMD_GRAM_VER_AD        0x21
#define LCD_CMD_RW_GRAM            0x22
#define LCD_CMD_POW_CTRL7          0x29
#define LCD_CMD_FRM_RATE_COL_CTRL  0x2B
#define LCD_CMD_GAMMA_CTRL1        0x30
#define LCD_CMD_GAMMA_CTRL2        0x31
#define LCD_CMD_GAMMA_CTRL3        0x32
#define LCD_CMD_GAMMA_CTRL4        0x35
#define LCD_CMD_GAMMA_CTRL5        0x36
#define LCD_CMD_GAMMA_CTRL6        0x37
#define LCD_CMD_GAMMA_CTRL7        0x38
#define LCD_CMD_GAMMA_CTRL8        0x39
#define LCD_CMD_GAMMA_CTRL9        0x3C
#define LCD_CMD_GAMMA_CTRL10       0x3D
#define LCD_CMD_HOR_START_AD       0x50
#define LCD_CMD_HOR_END_AD         0x51
#define LCD_CMD_VER_START_AD       0x52
#define LCD_CMD_VER_END_AD         0x53
#define LCD_CMD_GATE_SCAN_CTRL1    0x60
#define LCD_CMD_GATE_SCAN_CTRL2    0x61
#define LCD_CMD_GATE_SCAN_CTRL3    0x6A
#define LCD_CMD_PART_IMG1_DISP_POS 0x80
#define LCD_CMD_PART_IMG1_START_AD 0x81
#define LCD_CMD_PART_IMG1_END_AD   0x82
#define LCD_CMD_PART_IMG2_DISP_POS 0x83
#define LCD_CMD_PART_IMG2_START_AD 0x84
#define LCD_CMD_PART_IMG2_END_AD   0x85
#define LCD_CMD_PANEL_IF_CTRL1     0x90
#define LCD_CMD_PANEL_IF_CTRL2     0x92
#define LCD_CMD_PANEL_IF_CTRL3     0x93
#define LCD_CMD_PANEL_IF_CTRL4     0x95
#define LCD_CMD_PANEL_IF_CTRL5     0x97
#define LCD_CMD_PANEL_IF_CTRL6     0x98
//@}

 
/**@name 16-bit Color constants
 * @brief Color constants for use in (default) 16-bit drawing functions
 */
//@{
#define BLACK       0x0000
#define WHITE       0xFFFF
#define GRAY        0x8410
#define GRAY_DARK   0x4208
#define GRAY_LIGHT  0xC618
#define BLUE        0x001F
#define RED         0xF800
#define GREEN       0x07E0
#define CYAN        0x07FF
#define MAGENTA     0xF81F
#define YELLOW      0xFFE0 
#define ORANGE      0xFC00
//@}

/**@name 8-bit Color constants
 * @brief Color constants for use in 8-bit drawing functions
 */
//@{
#define BLACK_8BIT     0x00
#define WHITE_8BIT     0xFF
#define GRAY_8BIT      0xD5
#define YELLOW_8BIT    0xEE
#define ORANGE_8BIT    0xE4
#define RED_8BIT       0xE0
#define GREEN_8BIT     0xC7
#define BLUE_8BIT      0x9E
#define CYAN_8BIT      0xDF
#define PURPLE_8BIT    0xDC
//@}

/**@name Display cursor direction constants
 * @brief Specifies in what direction to draw pixels
 */
//@{
#define DIR_RIGHT_WRAP_DOWN  0x08
#define DIR_DOWN_WRAP_DOWN   0x20
#define DIR_LEFT_WRAP_DOWN   0x38
#define DIR_UP_WRAP_DOWN     0x10
#define DIR_RIGHT_WRAP_UP    0x18
#define DIR_DOWN_WRAP_UP     0x00
#define DIR_LEFT_WRAP_UP     0x28
#define DIR_UP_WRAP_UP       0x30
#define DIR_RIGHT            DIR_RIGHT_WRAP_DOWN
#define DIR_DOWN             DIR_DOWN_WRAP_DOWN
#define DIR_LEFT             DIR_LEFT_WRAP_DOWN
#define DIR_UP               DIR_UP_WRAP_DOWN
//@}

/// Macro to turn 8-bit color into 16-bit color
#define COLOR8TO16(color)    ((uint16_t) ((color & 0xFF) | ((color) << 8)))

/// Main display hardware functions are contained here
namespace PHNDisplayHW {

  /// The display width
  const uint16_t WIDTH  = 320;
  /// The display height
  const uint16_t HEIGHT = 240;
  /// The total amount of pixels of the display
  const uint32_t PIXELS = ((uint32_t) WIDTH * (uint32_t) HEIGHT);
  /// The display width that takes the slider into account
  const uint16_t WIDTH_SLIDER = (WIDTH + 32);

  /// Resets and then initializes the LCD screen registers for first use
  void init();
  /// Writes a single word of data to the LCD display
  void writeData(uint16_t data);
  /// Writes a single command to the LCD display
  void writeCommand(uint8_t cmd);
  /// Writes a command, and an argument, to the LCD display, to set up a register
  void writeRegister(uint8_t cmd, uint16_t arg);
  /// Reads a single word of data from the LCD display
  uint16_t readData();
  /// Writes a command, then reads the response from the LCD display
  uint16_t readRegister(uint8_t cmd);
  /// Sets up the register of the LCD to update the cursor coordinates
  void setCursor(uint16_t x, uint16_t y, uint8_t direction);
  /// Updates the cursor position, using a horizontal direction wrapping down
  void setCursor(uint16_t x, uint16_t y);
  /// Sets the viewport for the display
  void setViewport(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
  /// Converts RGB color into the 16-bit 565-color format used by the display
  color_t color565(uint8_t r, uint8_t g, uint8_t b);
  /// Calculates the average of two 16-bit 565 colors
  color_t colorAverage(color_t colorA, color_t colorB);
  /// Obtains the RED component of a 16-bit 565 color
  uint8_t color565Red(color_t color);
  /// Obtains the GREEN component of a 16-bit 565 color
  uint8_t color565Green(color_t color);
  /// Obtains the BLUE component of a 16-bit 565 color
  uint8_t color565Blue(color_t color);
  /// Reads raw touchscreen input information
  void readTouch(uint16_t *analogX, uint16_t *analogY, uint16_t *analogZ1, uint16_t *analogZ2);
  /// Reads touchscreen input, performs calibration and returns the x/y coordinate and the pressure
  void readTouch(uint16_t *touch_x, uint16_t *touch_y, float *pressure);
}

/// 8-bit display (drawing) logic for size and speed optimization freaks
namespace PHNDisplay8Bit {
  /// Writes out a single 8-bit color pixel
  void writePixel(uint8_t color);
  /// Writes out many 8-bit color pixels in bulk, length is the amount of pixels to write out
  void writePixels(uint8_t color, uint32_t length);
  /// Writes out many 8-bit color pixels in bulk, specifying how many lines to fill
  void writePixelLines(uint8_t color, uint8_t lines);
  /// Drawing a line with 8-bit color
  void drawLine(uint16_t x, uint16_t y, uint32_t length, uint8_t direction, uint8_t color);
  /// Drawing a rectangle with 8-bit color
  void drawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t color);
  /// Filling rectangle with 8-bit color
  void fillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t color);
  /// Fills the entire screen with 8-bit color
  void fill(uint8_t color);

  /// Fills the screen with all possible 8-bit colors (256 of them)
  void colorTest();
  
  /// Draws a String to the screen using the standard font
  void writeString(uint16_t x, uint16_t y, uint8_t scale, const char* text, uint8_t color0, uint8_t color1);
  /// Draws a Character to the screen using the standard font
  void writeChar(uint16_t x, uint16_t y, uint8_t scale, char c, uint8_t color0, uint8_t color1);

  /// Draws a 5x7 1-bit font character, the font data must be stored in FLASH
  void writeFont_1bit(uint16_t x, uint16_t y, uint8_t scale, const uint8_t* data, uint8_t color0, uint8_t color1);
  
  /// Drawing 1-bit images stored in RAM (not FLASH!)
  void writeImage_1bit(uint16_t x, uint16_t y, uint8_t width, uint8_t height, 
                       uint8_t scale, const uint8_t* data, uint8_t direction, uint8_t color0, uint8_t color1);
}

/// 16-bit display (drawing) logic for full color
namespace PHNDisplay16Bit {
  /// Writes out a single 16-bit color pixel
  void writePixel(uint16_t color);
  /// Writes out many 16-bit color pixels in bulk, length is the amount of pixels to write out
  void writePixels(uint16_t color, uint32_t length);
  /// Drawing a line with 16-bit color
  void drawLine(uint16_t x, uint16_t y, uint32_t length, uint8_t direction, uint16_t color);
  /// Fills the entire screen with 16-bit color
  void fill(uint16_t color);

  /// Fills the screen with all possible 16-bit colors (65536 of them)
  void colorTest();

  /// Draws a String to the screen using the standard font
  void writeString(uint16_t x, uint16_t y, uint8_t scale, const char* text, uint16_t color0, uint16_t color1);
  /// Draws a Character to the screen using the standard font
  void writeChar(uint16_t x, uint16_t y, uint8_t scale, char c, uint16_t color0, uint16_t color1);

  /// Draws a 5x7 1-bit font character, the font data must be stored in FLASH
  void writeFont_1bit(uint16_t x, uint16_t y, uint8_t scale, const uint8_t* data, uint16_t color0, uint16_t color1);

  /// Drawing 1-bit images stored in RAM (not FLASH!)
  void writeImage_1bit(uint16_t x, uint16_t y, uint8_t width, uint8_t height, 
                       uint8_t scale, const uint8_t* data, uint8_t direction, uint16_t color0, uint16_t color1);
}

/** @}*/

#endif