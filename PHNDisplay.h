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
 * @addtogroup Display
 * @brief The display module includes everything needed to make use of the TFT-LCD screen
 * 
 * You can make use of the global @link PHN_Display display @endlink to work with @link PHN_Widget widgets @endlink
 * or to perform manual drawing to the screen. It also features an automated system for reading touch input,
 * with screen calibration all set up behind the scenes.
 *
 * If all of this abstraction is too much for you, for example because you need to reduce code size, you can
 * instead make use of the @link PHNDisplayHardware.h hardware variant @endlink of the display. This is the low-level logic the display makes use of,
 * which is less user-friendly but does enable minimal code size approaches.
 * 
 * @{
 */

/**@file
 * @brief Contains the PHN_Display used for drawing, touch input and widgets
 */

#ifndef _PHN_DISPLAY_H_
#define _PHN_DISPLAY_H_

#include "PHNDisplayHardware.h"
#include "PHNSettings.h"
#include "PHNWidget.h"
#include "PHNDate.h"
#include <utility/PHNUtils.h>

/// Struct to hold the viewport information
typedef struct {
 public:
  uint16_t x, y, w, h;
} Viewport;

/// Struct to hold the touch screen input information
typedef struct {
  int x, y;
  float pressure;

  bool isPressed() const { return pressure > 0.0F; }
  
  bool isPressed(int x, int y, int width, int height) {
    return isPressed() && this->x >= x && this->y >= y && this->x < (x + width) && this->y < (y + height);
  }
} PressPoint;

/// Struct to hold the screen text drawing options
typedef struct {
  uint8_t textsize;
  uint16_t cursor_x, cursor_y;
  uint16_t cursor_x_start;
  color_t textcolor;
  color_t textbg;
  bool text_hasbg;
} TextOptions;

/// Struct to hold the header information of the LCD image format
typedef struct {
    uint8_t bpp;
    uint16_t width;
    uint16_t height;
    uint16_t colors;
    // color values
    // pixel data
} Imageheader_LCD;

/// Struct to hold the header information of the Bitmap image format
typedef struct {
  uint32_t size;
  uint16_t unused1;
  uint16_t unused2;
  uint32_t pixelDataOffset;
  uint32_t headerSize;
  uint32_t width;
  uint32_t height;
  uint16_t planes;
  uint16_t bitCount;
  uint32_t compression;
  uint32_t bitmapSize;
  uint32_t PPM_x;
  uint32_t PPM_y;
  uint32_t biClrUsed;
  uint32_t biClrImportant;
} Imageheader_BMP;

/// Struct to hold ARGB 32-bit color information used in bitmaps
typedef struct {
  uint8_t b;
  uint8_t g;
  uint8_t r;
  uint8_t a;
} Color_ARGB;

// Touchscreen constants
#define TFTLCD_TOUCH_PRESSURE_THRESHOLD 90 // Pressed down above this value
#define TFTLCD_TOUCH_PRESSDELAY 30 // Time in MS required before a press change is registered

// Wrap-around modes for setWrapMode(mode)
#define WRAPMODE_DOWN  0x0
#define WRAPMODE_UP    0x4

// Touch input smoothen function - touch change (delta) passes through here
#define TFTLCD_TOUCH_SMOOTH(x)  (0.01 * abs(x) * x)

/**
 * @brief Simplifies the use of the display with drawing, touch input and @link PHN_Widget widgets @endlink
 *
 * Includes a display instance which communicates with the LCD hardware.
 * With this class drawing shapes, text, images and other content is simplified.
 * In addition it hosts widgets which are automatically updated and drawn to screen.
 * Calling update() will update the touch input and then all widgets added.
 * This way a lively user interface can be easily implemented.
*/
class PHN_Display : public PHN_WidgetContainer {
 public:
  /// Is constructed globally, do not use (?), use 'display' global variable instead
  PHN_Display();
  /// Put the LCD-screen to sleep or wake it up again for reducing power consumption
  void setSleeping(bool sleeping);
  /// Changes the brightness of the LCD LED backlight, level being 0 - 256
  void setBacklight(int level);
  /// Updates the touch input, then all widgets attached to the display
  void update();
  /// Updates only the touch input, use this instead of update() to save code if not using widgets
  void updateTouch();
  /// Updates only the widgets, assuming no touch. Use this when the ADC is used for something else.
  void updateWidgets();
  /// Invalidates all added widgets, forcing them to redraw upon the next update
  void invalidate(void);
  /// Gets the width of the display - height when screen is rotated 90/270 degrees
  uint16_t width();
  /// Gets the height of the display - width when screen is rotated 90/270 degrees
  uint16_t height();
  /// Gets whether the width is greater than the height, and the screen is a widescreen
  bool isWidescreen(void);

  /**@brief Gets or sets the screen rotation transform
   * 
   * Possible values are 0, 1, 2 and 3 where 4 becomes 0 (wraps around)
   */
  void setScreenRotation(uint8_t rotation);
  /// Gets the screen rotation transform ranging [0..3]
  uint8_t getScreenRotation(void);

  /**@brief Sets the wrap-around mode when reaching screen/viewport borders.
   * 
   * Possible values: WRAPMODE_UP and WRAPMODE_DOWN
   */
  void setWrapMode(uint8_t mode);

  /**@brief Sets drawn pixel position to [x,y] and a horizontal direction
   * 
   * Is mainly for internal use, but can be used with the hardware LCD for custom use.
   */
  void goTo(uint16_t x, uint16_t y);
  /**@brief Sets drawn pixel position to [x,y] with a direction to draw in specified.
   * 
   * Is mainly for internal use, but can be used with the hardware LCD for custom use.
   * The direction ranges [0...3] and specifies in 90-degree angles.
   * A direction of 0 is to the right, 1 down, 2 left and 3 up.
   */
  void goTo(uint16_t x, uint16_t y, uint8_t direction);

  // Viewport functions
  /// Sets a viewport (rectangular area) to draw in
  void setViewport(Viewport viewport);
  /// Sets a viewport (rectangular area) to draw in
  void setViewport(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
  /// Sets a viewport (rectangular area) to draw in
  void setViewportRelative(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
  /// Resets the viewport to draw in the entire screen
  void resetViewport(void);
  /// Gets the current viewport that is applied
  Viewport getViewport(void);
  /// Gets the total amount of pixels the viewport area currently occupies
  uint32_t getViewportArea(void);

  // Screen touch functions
  /// Gets the location and pressure of the previous touch state
  PressPoint getTouchLast(void);
  /// Gets the location and pressure of the current touch state
  PressPoint getTouch(void);
  /// Gets the location and pressure of the last time the screen was touched down
  PressPoint getTouchStart(void);
  /// Gets whether the screen is touched down on
  bool isTouched(void);
  /// Gets whether the screen was previously not touched but now is
  bool isTouchDown(void);
  /// Gets whether the screen was previously touched but now no more
  bool isTouchUp(void);
  /// Gets whether a particular area is touched down on
  bool isTouched(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
  /// Gets whether a particular area was previously not touched but now is
  bool isTouchEnter(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
  /// Gets whether a particular area was previously touched but is now no more
  bool isTouchLeave(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
  /// Gets whether a particular area was previously touched and was then released shortly to engage a click
  bool isTouchClicked(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
  /// Gets whether a touch change occurred for an area, such as entering/leaving/clicking
  bool isTouchChange(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

  // Slider touch functions
  /// Gets the current slider value ranging [0..1]
  float getSlider(void);
  /// Gets the slider value ranging [0..1] obtained at the start of a press
  float getSliderStart(void);
  /// Gets whether the slider is touched
  bool isSliderTouched(void);
  /// Gets whether the slider is touched and was not before
  bool isSliderTouchDown(void);
  /// Gets whether the slider is no longer touched and was before
  bool isSliderTouchUp(void);

  // Shape drawing
  /// Draws a single pixel to the screen
  void drawPixel(uint16_t x, uint16_t y, color_t color);
  /// Fills the entire screen or viewport
  void fill(color_t color);
  /// Draws a line from one point to another
  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, color_t color);

  /**@brief Draws a straight line starting at [x, y], moving into the direction.
   * 
   * An alternative to drawLine that can draw just a little faster.
   * The direction ranges [0...3] and specifies in 90-degree angles.
   * A direction of 0 is to the right, 1 down, 2 left and 3 up.
   */
  void drawStraightLine(uint16_t x, uint16_t y, uint16_t length, uint8_t direction, color_t color);
  
  /// Draws a straight line down starting at [x, y]
  void drawVerticalLine(uint16_t x, uint16_t y, uint16_t length, color_t color);
  /// Draws a straight line to the right starting at [x, y]
  void drawHorizontalLine(uint16_t x, uint16_t y, uint16_t length, color_t color);
  /// Draws a triangle connecting the three points specified
  void drawTriangle(uint16_t x0, uint16_t y0,
        uint16_t x1, uint16_t y1,
        uint16_t x2, uint16_t y2, color_t color);
  /// Fills in a triangle connecting the three points specified
  void fillTriangle(int32_t x0, int32_t y0,
        int32_t x1, int32_t y1,
        int32_t x2, int32_t y2, 
        color_t color);
  /// Draws a rectangle at [x, y] with dimensions [w, h]
  void drawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, color_t color);
  /// Fills a rectangle at [x, y] with dimensions [w, h]
  void fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, color_t color);
  /// Fills a rectangle at [x, y] with dimensions [w, h], the outside set to a 1-pixel wide border
  void fillBorderRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, color_t color, color_t borderColor);
  /// Draws a rounded rectangle at [x, y] with dimensions [w, h] with the rounded radius
  void drawRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t radius, color_t color);
  /// Fills a rounded rectangle at [x, y] with dimensions [w, h] with the rounded radius
  void fillRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t radius, color_t color);
  /// Draws a circle at [x, y] of radius [r]
  void drawCircle(uint16_t x, uint16_t y, uint16_t r, color_t color);
  /// Fills a circle at [x, y] of radius [r]
  void fillCircle(uint16_t x, uint16_t y, uint16_t r, color_t color);

  /* ====================================== TEXT DRAWING ======================================== */

  /* Sets the cursor pixel x/y where new text will be drawn */
  /// Gets the current text-drawing cursor x-coordinate
  const uint16_t getCursorX(void) {return textOpt.cursor_x;}
  /// Gets the current text-drawing cursor y-coordinate
  const uint16_t getCursorY(void) {return textOpt.cursor_y;}
  /// Sets the current text-drawing cursor [x,y] coordinate
  void setCursor(uint16_t x, uint16_t y);
  /// Sets the current text-drawing cursor one newline down at the x-coordinate specified
  void setCursorDown(uint16_t x);

  /// Sets the absolute horizontal scrolling value
  void setScroll(int value);
  /// Gets the absolute horizontal scrolling value
  uint16_t getScroll() const { return _scroll; }
  /// Scrolls the screen horizontally by an offset
  void scroll(int offset) { setScroll((int) _scroll + offset); }

  /* Options for printing text */
  /// Sets the text color to a color, with a transparent background
  void setTextColor(color_t c);
  /// Sets the text color to a color, with a specified background color
  void setTextColor(color_t c, color_t bg);
  /// Sets the text size ranging [1..20]
  void setTextSize(uint8_t s);
  /// Gets the current text background color set
  color_t getTextBackground(void) {return textOpt.textbg;}
  /// Gets the current text foreground color set
  color_t getTextColor(void) {return textOpt.textcolor;}
  /// Gets the current text size set
  uint8_t getTextSize(void) {return textOpt.textsize;}
  /// Gets all current text rendering options
  TextOptions getTextOptions(void) { return textOpt; }
  /// Sets all current text rendering options
  void setTextOptions(TextOptions opt) { textOpt = opt; }

  /* Draw text at specific coordinates */
  /// Draws a single character at [x, y] of an optional size specified
  void drawChar(uint16_t x, uint16_t y, char c, uint8_t s = 1);
  /// Draws character data (7x5 size) at [x, y] of an optional size specified
  void drawCharMem(uint16_t x, uint16_t y, const uint8_t* font_char, uint8_t s = 1);
  /// Draws a String of characters at [x, y] of an optional size specified
  void drawString(uint16_t x, uint16_t y, const char* text, uint8_t s = 1);
  /**@brief Draws a String of characters in the area [x, y, width, height]
   *
   * Draws the text in the middle of the rectangular area specified.
   * The size to draw at is automatically calculated to fit the text.
   */
  void drawStringMiddle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const char* text);

  /// Prints hours and minutes part of a date
  void printShortTime(Date date);
  /// Prints hours, minutes and seconds part of a date
  void printTime(Date date);
  /// Prints the month and day of a date
  void printDate(Date date);
  /// Prints a character stored in memory in 7x5 size format
  void printMem(const uint8_t* font_char);

  /// Prints out text in a convenient short way
  void debugPrint(uint16_t x, uint16_t y, uint8_t size, const char* text, uint8_t padding = 0);
  /// Prints out a number to the screen in a convenient short way
  void debugPrint(uint16_t x, uint16_t y, uint8_t size, int value);
  /// Prints out a float to the screen in a convenient short way
  void debugPrint(uint16_t x, uint16_t y, uint8_t size, float value);

  /**@name Printing and writing functions
   * @brief Functions imported from Print.h 
   *
   * Inheriting Print.h means the virtual write function is always compiled.
   * This resulted in a significant increase of the final binary.
   * For this reason these functions are instead copied here.
   * 
   * The exact same Arduino Print documentation applies here.
   */
  //@{
  size_t printNumber(unsigned long, uint8_t);
  size_t printFloat(double, uint8_t);
  size_t write(const uint8_t *buffer, size_t size);
  size_t write(const char *str);
  size_t write(uint8_t c);

  size_t print(const __FlashStringHelper *);
  size_t print(const String &);
  size_t print(const char[]);
  size_t print(char);
  size_t print(unsigned char, int = DEC);
  size_t print(int, int = DEC);
  size_t print(unsigned int, int = DEC);
  size_t print(long, int = DEC);
  size_t print(unsigned long, int = DEC);
  size_t print(double, int = 2);

  size_t println(const __FlashStringHelper *);
  size_t println(const String &s);
  size_t println(const char[]);
  size_t println(char);
  size_t println(unsigned char, int = DEC);
  size_t println(int, int = DEC);
  size_t println(unsigned int, int = DEC);
  size_t println(long, int = DEC);
  size_t println(unsigned long, int = DEC);
  size_t println(double, int = 2);
  size_t println(void);
  //@}

  /* Image drawing routines */
  /**@name Image Drawing Functions
   * @brief Draws .BMP or .LCD image data to the screen
   *
   * Send in a stream of data containing the .BMP or .LCD image contents.
   * For this you can make use of the SD File, FlashMemoryStream and MemoryStream.
   * This way images can be stored on a variety of media, even Serial!
   *
   * You can draw the image with a r/g/b/brightness modifier, you can use a list
   * of colors (colormap) or you can write your own color-converting function to use.
   * If you wish to alter the colors of an image, or in general re-use the same image
   * for different styles, you can use these extra functions.
   *
   * Be aware that reading image data can be fairly slow, so 'compress' your images
   * into 1-bit (2 colors), 2-bit (4 colors), 4-bit (16 colors) or 8-bit (256 colors)
   * before use.
   *
   * The .BMP format only supports 8-bit and 24-bit color formats. It is recommended
   * to convert your images into .LCD using the Phoenard toolkit before use.
   */
  //@{
  void drawImage(Stream &imageStream, int x, int y);
  void drawImage(Stream &imageStream, int x, int y, float brightness);
  void drawImage(Stream &imageStream, int x, int y, float cr, float cg, float cb);
  void drawImageColorFunc(Stream &imageStream, int x, int y, void (*color)(uint8_t*, uint8_t*, uint8_t*));
  void drawImageColorMap(Stream &imageStream, int x, int y, color_t *colorMapInput);
  //@}
 private:
  void drawImageMain(Stream &imageStream, int x, int y, void (*color)(uint8_t*, uint8_t*, uint8_t*), color_t *colorMapInput);
  void drawCircleHelper(uint16_t x0, uint16_t y0, uint16_t r, uint8_t corner, color_t color);
  void fillCircleHelper(uint16_t x0, uint16_t y0, uint16_t r, uint8_t corner, uint16_t delta, color_t color);

  uint8_t screenRot;
  uint8_t wrapMode;
  uint16_t _width, _height;
  uint16_t _scroll;
  Viewport _viewport;
  void (*cgramFunc)(uint16_t*, uint16_t*);
  void (*cgramFunc_inv)(uint16_t*, uint16_t*);

  // Text drawing
  TextOptions textOpt;

  // Touchscreen input variables
  bool touchInputLive, touchInput, touchInputSlider;
  long touchInputLastStable;

  // Touchscreen API variables
  PressPoint touchStart, touchLive, touchLast;
  bool touchClicked;
  bool sliderDown, sliderWasDown;
  float sliderStart, sliderLive;
};

/// Global variable from which the display functions can be accessed
extern PHN_Display display;

/** @}*/

#endif
