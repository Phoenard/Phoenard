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

#include "PHNDisplay.h"
#include "PHNDisplayFont.c"

static const uint8_t DIR_TRANSFORM[] = {DIR_RIGHT_WRAP_DOWN, DIR_DOWN_WRAP_DOWN,
                                          DIR_LEFT_WRAP_DOWN, DIR_UP_WRAP_DOWN,
                                          DIR_RIGHT_WRAP_UP, DIR_DOWN_WRAP_UP,
                                          DIR_LEFT_WRAP_UP, DIR_UP_WRAP_UP};

// ================ Functions used to transform x/y based on screen rotation==================

static void calcGRAMPosition_0(uint16_t *posx, uint16_t *posy) {
}
static void calcGRAMPosition_1(uint16_t *posx, uint16_t *posy) {
  swap(*posx, *posy);
  *posx = PHNDisplayHW::WIDTH - 1 - *posx;
}
static void calcGRAMPosition_1_inv(uint16_t *posx, uint16_t *posy) {
  *posx = PHNDisplayHW::WIDTH - 1 - *posx;
  swap(*posx, *posy);
}
static void calcGRAMPosition_2(uint16_t *posx, uint16_t *posy) {
  *posx = PHNDisplayHW::WIDTH - 1 - *posx;
  *posy = PHNDisplayHW::HEIGHT - 1 - *posy;
}
static void calcGRAMPosition_3(uint16_t *posx, uint16_t *posy) {
  swap(*posx, *posy);
  *posy = PHNDisplayHW::HEIGHT - 1 - *posy;
}
static void calcGRAMPosition_3_inv(uint16_t *posx, uint16_t *posy) {
  *posy = PHNDisplayHW::HEIGHT - 1 - *posy;
  swap(*posx, *posy);
}
// ===========================================================================================
                                          
// Initialize display here
PHN_Display display;

PHN_Display::PHN_Display() {
  /* LCD Initialization for first-time use */
  screenRot = 0;
  _scroll = 0;
  _width = PHNDisplayHW::WIDTH;
  _height = PHNDisplayHW::HEIGHT;
  _viewport.x = 0;
  _viewport.y = 0;
  _viewport.w = _width;
  _viewport.h = _height;
  textOpt.text_hasbg = false;
  textOpt.textcolor = WHITE;
  textOpt.textsize = 1;
  cgramFunc = calcGRAMPosition_0;
  cgramFunc_inv = calcGRAMPosition_0;
}

void PHN_Display::setSleeping(bool sleeping) {
  if (sleeping) {
    for (int i=255;i>-1;i--){
      analogWrite(TFTLCD_BL_PIN,i);
      delay(2);
    }
    PHNDisplayHW::writeRegister(LCD_CMD_POW_CTRL1,0x17b2);
  } else {
    PHNDisplayHW::writeRegister(LCD_CMD_POW_CTRL1,0x17b0);
    for (int i=-1;i<256;i++){
      analogWrite(TFTLCD_BL_PIN,i);
      delay(2);
    }
  }
}

uint16_t PHN_Display::width() {
  return _viewport.w;
}

uint16_t PHN_Display::height() {
  return _viewport.h;
}

bool PHN_Display::isWidescreen() {
  return _width > _height;
}

void PHN_Display::setScreenRotation(uint8_t rotation) {
  screenRot = rotation & 0x3;
  // Update the x/y transfer function
  switch (screenRot) {
  case 0:
    cgramFunc = calcGRAMPosition_0;
    cgramFunc_inv = calcGRAMPosition_0;
    break;
  case 1:
    cgramFunc = calcGRAMPosition_1;
    cgramFunc_inv = calcGRAMPosition_1_inv;
    break;
  case 2:
    cgramFunc = calcGRAMPosition_2;
    cgramFunc_inv = calcGRAMPosition_2;
    break;
  case 3:
    cgramFunc = calcGRAMPosition_3;
    cgramFunc_inv = calcGRAMPosition_3_inv;
    break;
  }
  // Update screen dimensions
  _width = PHNDisplayHW::WIDTH * (1-(screenRot & 0x1)) + PHNDisplayHW::HEIGHT * (screenRot & 0x1);
  _height = PHNDisplayHW::HEIGHT * (1-(screenRot & 0x1)) + PHNDisplayHW::WIDTH * (screenRot & 0x1);
  // Reset viewport (original bounds are no longer valid!)
  resetViewport();
}

uint8_t PHN_Display::getScreenRotation() {
  return screenRot & 0x3;
}

void PHN_Display::setWrapMode(uint8_t mode) {
  wrapMode = mode;
}

void PHN_Display::goTo(uint16_t x, uint16_t y) {
  goTo(x, y, 0);
}

void PHN_Display::goTo(uint16_t x, uint16_t y, uint8_t direction) {
  // Apply screen rotation transform to x/y
  x += _viewport.x;
  y += _viewport.y;
  cgramFunc(&x, &y);
  // Write to the display
  PHNDisplayHW::setCursor(x, y, DIR_TRANSFORM[((direction + screenRot) & 0x3) | wrapMode]);
}

void PHN_Display::resetViewport() {
  setViewport(0, 0, _width, _height);
}

void PHN_Display::setViewportRelative(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  setViewport(_viewport.x + x, _viewport.y + y, w, h);
}

void PHN_Display::setViewport(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  // Update viewport
  Viewport newViewport;
  newViewport.x = x;
  newViewport.y = y;
  newViewport.w = w;
  newViewport.h = h;
  setViewport(newViewport);
}

void PHN_Display::setViewport(Viewport viewport) {
  _viewport = viewport;
  // Start/end values to be set
  uint16_t x1 = viewport.x, y1 = viewport.y;
  uint16_t x2 = x1 + viewport.w - 1, y2 = y1 + viewport.h - 1;
  // Apply screen transforms
  cgramFunc(&x1, &y1);
  cgramFunc(&x2, &y2);
  // Apply to the display
  PHNDisplayHW::setViewport(x1, y1, x2, y2);
}

Viewport PHN_Display::getViewport() {
  return _viewport;
}

uint32_t PHN_Display::getViewportArea() {
  return (uint32_t) _viewport.w * (uint32_t) _viewport.h;
}

// draw a triangle!
void PHN_Display::drawTriangle(uint16_t x0, uint16_t y0,
        uint16_t x1, uint16_t y1,
        uint16_t x2, uint16_t y2, color_t color) {
  drawLine(x0, y0, x1, y1, color);
  drawLine(x1, y1, x2, y2, color);
  drawLine(x2, y2, x0, y0, color); 
}

void PHN_Display::fillTriangle ( int32_t x0, int32_t y0,
        int32_t x1, int32_t y1,
        int32_t x2, int32_t y2, color_t color) {
  if (y0 > y1) {
    swap(y0, y1); swap(x0, x1);
  }
  if (y1 > y2) {
    swap(y2, y1); swap(x2, x1);
  }
  if (y0 > y1) {
    swap(y0, y1); swap(x0, x1);
  }

  int32_t dx1, dx2, dx3; // Interpolation deltas
  int32_t sx1, sx2, sy; // Scanline co-ordinates

  sx2=(int32_t)x0 * (int32_t)1000; // Use fixed point math for x axis values
  sx1 = sx2;
  sy=y0;

  // Calculate interpolation deltas
  if (y1-y0 > 0) dx1=((x1-x0)*1000)/(y1-y0);
    else dx1=0;
  if (y2-y0 > 0) dx2=((x2-x0)*1000)/(y2-y0);
    else dx2=0;
  if (y2-y1 > 0) dx3=((x2-x1)*1000)/(y2-y1);
    else dx3=0;

  // Render scanlines (horizontal lines are the fastest rendering method)
  if (dx1 > dx2) {
    for(; sy<=y1; sy++, sx1+=dx2, sx2+=dx1) {
      drawHorizontalLine(sx1/1000, sy, (sx2-sx1)/1000, color);
    }
    sx2 = x1*1000;
    sy = y1;
    for(; sy<=y2; sy++, sx1+=dx2, sx2+=dx3) {
      drawHorizontalLine(sx1/1000, sy, (sx2-sx1)/1000, color);
    }
  } else {
    for(; sy<=y1; sy++, sx1+=dx1, sx2+=dx2) {
      drawHorizontalLine(sx1/1000, sy, (sx2-sx1)/1000, color);
    }
    sx1 = x1*1000;
    sy = y1;
    for(; sy<=y2; sy++, sx1+=dx3, sx2+=dx2) {
      drawHorizontalLine(sx1/1000, sy, (sx2-sx1)/1000, color);
    }
  }
}

// draw a rectangle
void PHN_Display::drawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, 
          color_t color) {
  // smarter version
  drawHorizontalLine(x, y, w, color);
  drawHorizontalLine(x, y+h-1, w, color);
  drawVerticalLine(x, y, h, color);
  drawVerticalLine(x+w-1, y, h, color);
}

// draw a rounded rectangle
void PHN_Display::drawRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r,
         color_t color) {
  // smarter version
  drawHorizontalLine(x+r, y, w-2*r, color);
  drawHorizontalLine(x+r, y+h-1, w-2*r, color);
  drawVerticalLine(x, y+r, h-2*r, color);
  drawVerticalLine(x+w-1, y+r, h-2*r, color);
  // draw four corners
  drawCircleHelper(x+r, y+r, r, 1, color);
  drawCircleHelper(x+w-r-1, y+r, r, 2, color);
  drawCircleHelper(x+w-r-1, y+h-r-1, r, 4, color);
  drawCircleHelper(x+r, y+h-r-1, r, 8, color);
}

// fill a rounded rectangle
void PHN_Display::fillRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r,
         color_t color) {
  // smarter version
  fillRect(x+r, y, w-2*r, h, color);

  // draw four corners
  fillCircleHelper(x+w-r-1, y+r, r, 1, h-2*r-1, color);
  fillCircleHelper(x+r, y+r, r, 2, h-2*r-1, color);
}

// fill a circle
void PHN_Display::fillCircle(uint16_t x0, uint16_t y0, uint16_t r, color_t color) {
  drawVerticalLine(x0, y0-r, 2*r+1, color);
  fillCircleHelper(x0, y0, r, 3, 0, color);
}

// used to do circles and roundrects!
void PHN_Display::fillCircleHelper(uint16_t x0, uint16_t y0, uint16_t r, uint8_t cornername, uint16_t delta,
      color_t color) {

  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
  
    if (cornername & 0x1) {
      drawVerticalLine(x0+x, y0-y, 2*y+1+delta, color);
      drawVerticalLine(x0+y, y0-x, 2*x+1+delta, color);
    }
    if (cornername & 0x2) {
      drawVerticalLine(x0-x, y0-y, 2*y+1+delta, color);
      drawVerticalLine(x0-y, y0-x, 2*x+1+delta, color);
    }
  }
}


// draw a circle outline

void PHN_Display::drawCircle(uint16_t x0, uint16_t y0, uint16_t r, 
      color_t color) {
  drawPixel(x0, y0+r, color);
  drawPixel(x0, y0-r, color);
  drawPixel(x0+r, y0, color);
  drawPixel(x0-r, y0, color);

  drawCircleHelper(x0, y0, r, 0xF, color);
}

void PHN_Display::drawCircleHelper(uint16_t x0, uint16_t y0, uint16_t r, uint8_t cornername,
      color_t color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;


  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    if (cornername & 0x4) {
      drawPixel(x0 + x, y0 + y, color);
      drawPixel(x0 + y, y0 + x, color);
    } 
    if (cornername & 0x2) {
      drawPixel(x0 + x, y0 - y, color);
      drawPixel(x0 + y, y0 - x, color);
    }
    if (cornername & 0x8) {
      drawPixel(x0 - y, y0 + x, color);
      drawPixel(x0 - x, y0 + y, color);
    }
    if (cornername & 0x1) {
      drawPixel(x0 - y, y0 - x, color);
      drawPixel(x0 - x, y0 - y, color);
    }
  }
}

// fill a rectangle
void PHN_Display::fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, 
          color_t fillcolor) {

  if (w>>4 && h>>4) {
    // Use viewport for large surface areas
    Viewport oldViewport = getViewport();
    setViewportRelative(x, y, w, h);
    fill(fillcolor);
    setViewport(oldViewport);
  } else if (h >= w) {
    // Draw vertical lines when height > width
    for (uint16_t wx = 0; wx < w; wx++) {
      drawVerticalLine(x+wx, y, h, fillcolor);
    }
  } else {
    // Draw horizontal lines otherwise
    for (uint16_t wy = 0; wy < h; wy++) {
      drawHorizontalLine(x, y+wy, w, fillcolor);
    }
  }
}

void PHN_Display::drawVerticalLine(uint16_t x, uint16_t y, uint16_t length, color_t color) {
  drawStraightLine(x, y, length, 1, color);
}

void PHN_Display::drawHorizontalLine(uint16_t x, uint16_t y, uint16_t length, color_t color) {
  drawStraightLine(x, y, length, 0, color);
}

void PHN_Display::drawStraightLine(uint16_t x, uint16_t y, uint16_t length, uint8_t direction, color_t color) {
  goTo(x, y, direction);
  PHNDisplay16Bit::writePixels(color, length);
}

// bresenham's algorithm with optimizations
void PHN_Display::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, color_t color) {
  if (x0 == x1) {
    if (y1 > y0) {
      drawVerticalLine(x0, y0, (y1 - y0), color);
    } else {
      drawVerticalLine(x0, y1, (y0 - y1), color);
    }
    return;
  }
  if (y0 == y1) {
    if (x1 > x0) {
      drawHorizontalLine(x0, y0, (x1 - x0), color);
    } else {
      drawHorizontalLine(x1, y0, (x0 - x1), color);
    }
    return;
  }

  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    swap(x0, y0);
    swap(x1, y1);
  }

  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  //dy = abs(y1 - y0);
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int16_t ystep;

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;}

  for (; x0<=x1; x0++) {
    if (steep) {
      drawPixel(y0, x0, color);
    } else {
      drawPixel(x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

void PHN_Display::fill(color_t color) {
  goTo(0, 0, 0);
  PHNDisplay16Bit::writePixels(color, getViewportArea());
}

void PHN_Display::drawPixel(uint16_t x, uint16_t y, color_t color) {
  goTo(x, y);
  PHNDisplay16Bit::writePixel(color);
}

// ======================== Slider touch ===========================

float PHN_Display::getSlider() {
  return sliderLive;
}
float PHN_Display::getSliderStart() {
  return sliderStart;
}

bool PHN_Display::isSliderTouched() {
  return sliderDown;
}
bool PHN_Display::isSliderTouchDown() {
  return sliderDown && !sliderWasDown;
}
bool PHN_Display::isSliderTouchUp() {
  return !sliderDown && sliderWasDown;
}

// ======================== Display touch ==========================

PressPoint PHN_Display::getTouch() {
  return touchLive;
}
PressPoint PHN_Display::getTouchStart() {
  return touchStart;
}

PressPoint PHN_Display::getTouchLast() {
  return touchLast;
}

bool PHN_Display::isTouched() {
  return touchLive.pressure;
}
bool PHN_Display::isTouched(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
  return touchLive.isPressed(x, y, width, height);
}

bool PHN_Display::isTouchDown() {
  return !touchLast.pressure && touchLive.pressure;
}
bool PHN_Display::isTouchEnter(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
  // Now pressed down and it wasn't previously?
  return touchLive.isPressed(x, y, width, height) && !touchLast.isPressed(x, y, width, height);
}

bool PHN_Display::isTouchUp() {
  return touchLast.pressure && !touchLive.pressure;
}
bool PHN_Display::isTouchLeave(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
  // Was touched down before but is now no longer?
  return touchLast.isPressed(x, y, width, height) && !touchLive.isPressed(x, y, width, height);
}

bool PHN_Display::isTouchClicked(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
  // Area was previously touched but the touch is now released?
  return touchClicked && touchLast.isPressed(x, y, width, height);
}

bool PHN_Display::isTouchChange(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
  bool touched_old = touchLast.isPressed(x, y, width, height);
  bool touched_new = touchLive.isPressed(x, y, width, height);
  return (touchClicked && touched_old) || (touched_old != touched_new);
}

void PHN_Display::update() {
  updateTouch();
  updateWidgets(true, true, false);
}

void PHN_Display::updateTouch() {
  uint16_t touch_x, touch_y;
  float touch_pressure;

  // Store the previous state
  touchLast = touchLive;

  // Read the touch input
  PHNDisplayHW::readTouch(&touch_x, &touch_y, &touch_pressure);
  touchLive.pressure = (int) touch_pressure;

  // Update live touched down state
  if (touchLive.pressure >= TFTLCD_TOUCH_PRESSURE_THRESHOLD) {
    // Touch input received
    touchInputLive = true;
    
    // Pressed inside the screen, or pressed the slider?
    touchInputSlider = (touch_x >= PHNDisplayHW::WIDTH);
    
    if (touchInputSlider) {
      // Update slider value
      sliderLive = (float) touch_y / (float) PHNDisplayHW::HEIGHT;

      // Should reach full 0 - 1 easily, this ensures it
      sliderLive = 1.1*sliderLive-0.05;
      if (sliderLive > 1.0)
        sliderLive = 1.0;
      else if (sliderLive < 0.0)
        sliderLive = 0.0;

      // Reverse the slider for some of the rotation angles
      if (getScreenRotation() == 0 || getScreenRotation() == 1)
        sliderLive = 1.0 - sliderLive;
    } else {
      // Transform the touched x/z values to display space
      cgramFunc_inv(&touch_x, &touch_y);

      // Apply a smoothing if touched previously
      if (touchLast.pressure) {
        // Calculate the change in touch value
        int dx = ((int) touch_x - (int) touchLive.x);
        int dy = ((int) touch_y - (int) touchLive.y);

        // Apply a magic formula to smoothen changes
        touchLive.x += TFTLCD_TOUCH_SMOOTH(dx);
        touchLive.y += TFTLCD_TOUCH_SMOOTH(dy);
      } else {
        // Set it instantly
        touchLive.x = touch_x;
        touchLive.y = touch_y;
      }
    }
  } else if (touchLive.pressure == 0) {
    // No longer pressed when pressure reaches 0
    touchInputLive = false;
  }

  // When unchanging for delay, assume correct state
  if (touchInputLive == touchInput) {
    touchInputLastStable = millis();
  } else if ((millis() - touchInputLastStable) > TFTLCD_TOUCH_PRESSDELAY) {
    touchInputLastStable = millis();
    touchInput = touchInputLive;
  }

  // Update API state
  sliderWasDown = sliderDown;
  sliderDown = touchInput && touchInputSlider;
  if (!touchInput || touchInputSlider) {
    touchLive.pressure = 0;
  }

  // Further point updates on state changes
  if (touchLive.pressure && !touchLast.pressure) {
    touchStart = touchLive;
  }
  if (sliderDown && !sliderWasDown) {
    sliderStart = sliderLive;
  }
  
  // If press was released, mark it with a flag just this time
  if (!touchLive.pressure && touchLast.pressure) {
     // If clicked already handled, undo it and proceed with normal leave logical_and
     if (touchClicked) {
       touchClicked = false;
     } else {
       touchClicked = true;
       // Clicking: restore the 'last' state for next time
       touchLive = touchLast;
     }
  } else {
    touchClicked = false;
  }
}

/* =========================== TEXT DRAWING ============================== */

void PHN_Display::setCursorDown(uint16_t x) {
  setCursor(x, textOpt.cursor_y + textOpt.textsize*8);
}

void PHN_Display::setCursor(uint16_t x, uint16_t y) {
  textOpt.cursor_x = x;
  textOpt.cursor_x_start = x;
  textOpt.cursor_y = y;
}

void PHN_Display::setTextSize(uint8_t s) {
  textOpt.textsize = s;
}

void PHN_Display::setTextColor(color_t c) {
  textOpt.textcolor = c;
  textOpt.text_hasbg = false;
}

void PHN_Display::setTextColor(color_t c, color_t bg) {
  textOpt.textcolor = c;
  textOpt.textbg = bg;
  textOpt.text_hasbg = true;
}

void PHN_Display::setScroll(int value) {
  value %= 320;
  if (value < 0) {
    value += 320;
  }
  _scroll = value;
  PHNDisplayHW::writeRegister(LCD_CMD_GATE_SCAN_CTRL3, _scroll);
}

size_t PHN_Display::write(uint8_t c) {
  if (c == '\n') {
    textOpt.cursor_y += textOpt.textsize*8;
    textOpt.cursor_x = textOpt.cursor_x_start;
  } else if (c == '\r') {
    // skip em
  } else {
    drawChar(textOpt.cursor_x, textOpt.cursor_y, c, textOpt.textsize);
    textOpt.cursor_x += textOpt.textsize*6;
  }
  return 0;
}

void PHN_Display::printShortTime(Date date) {
  if (date.hour < 10)
    print('0');
  print(date.hour);
  print(':');
  if (date.minute < 10)
    print('0');
  print(date.minute);
}

void PHN_Display::printTime(Date date) {
  printShortTime(date);
  print(':');
  if (date.second < 10)
    print('0');
  print(date.second);
}

void PHN_Display::printDate(Date date) {
  if (date.day < 10)
    print('0');
  print(date.day);
  print('/');
  if (date.month < 10)
    print('0');
  print(date.month);
  print('/');
  if (date.year < 10)
    print('0');
  print(date.year);
}

void PHN_Display::printMem(const uint8_t* font_char) {
  drawCharMem(textOpt.cursor_x, textOpt.cursor_y, font_char, textOpt.textsize);
  write(' ');
}

void PHN_Display::drawStringMiddle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const char* text) {

  // Calculate the total amount of rows and columns the text displays
  unsigned int textCols = 0;
  unsigned int textCols_tmp = 0;
  unsigned int textRows = 1;
  const char* text_p = text;
  while (*text_p) {
    if (*text_p == '\n') {
      textRows++;
      if (textCols_tmp > textCols) textCols = textCols_tmp;
      textCols_tmp = 0;
    } else {
      textCols_tmp++;
    }
    text_p++;
  }
  if (textCols_tmp > textCols) textCols = textCols_tmp;

  // Draw text at an appropriate size to fit the rectangle
  unsigned int textsize = 1;
  while ((textCols * (textsize+1) * 6 + 2) < width && (textRows * (textsize+1) * 8 + 2) < height) {
    textsize++;
  }
  
  // Find offset to use to draw in the middle
  unsigned int textWidth = textCols * textsize * 6;
  unsigned int textHeight = textRows * textsize * 8;
  unsigned text_x = x + ((width - textWidth) >> 1);
  unsigned text_y = y + ((height - textHeight) >> 1);
  setTextSize(textsize);
  setCursor(text_x, text_y);
  print(text);
}

void PHN_Display::drawString(uint16_t x, uint16_t y, const char *c, uint8_t size) {
  uint16_t c_x = x;
  uint16_t c_y = y;
  while (*c) {
    if (*c == '\n') {
      c_x = x;
      c_y += size*8;
    } else {
      drawChar(c_x, c_y, *c, size);
      c_x += size*6;
    }
    c++;
  }
}

// draw a character
void PHN_Display::drawChar(uint16_t x, uint16_t y, char c, uint8_t size) {
  drawCharMem(x, y, font_5x7+(c*5), size);
}

void PHN_Display::drawCharMem(uint16_t x, uint16_t y, const uint8_t* font_char, uint8_t size) {
  // If transparent background, make use of a (slower) cube drawing algorithm
  // For non-transparent backgrounds, make use of the faster 1-bit image drawing function
  if (textOpt.text_hasbg) {
    // Use standard internal minimal drawing function
    PHNDisplay16Bit::writeFont_1bit(x, y, size, font_char, textOpt.textbg, textOpt.textcolor);
  } else {
    // Draw in vertical 'dot' chunks for each 5 columns
    // Empty (0) data 'blocks' are skipped leaving them 'transparent'
    uint8_t cx, cy;
    uint8_t line;
    uint16_t pcount;
    for (cx =0; cx<5; cx++) {
      line = pgm_read_byte(font_char++);
      cy = 0;
      while (line) {
        if (line & 0x1) {
          for (pcount = 0; line & 0x1; pcount++) {
            line >>= 1;
          }
          fillRect(x+cx*size, y+cy*size, size, size*pcount, textOpt.textcolor);
          cy += pcount;
        } else {
          line >>= 1;
          cy++;
        }
      }
    }
  }
}

void PHN_Display::debugPrint(uint16_t x, uint16_t y, uint8_t size, const char* text, uint8_t padding) {
  // Store old options
  TextOptions oldTextOpt = textOpt;

  // Set draw parameters
  setCursor(x, y);
  setTextSize(size);
  setTextColor(WHITE, BLACK);

  // Draw the text
  print(text);  

  // Pad the end with spaces
  uint8_t len = strlen(text);
  while (len < padding) {
    print(' ');
    len++;
  }

  // Restore text options
  textOpt = oldTextOpt;
}

void PHN_Display::debugPrint(uint16_t x, uint16_t y, uint8_t size, int value) {
  char buff[7];
  itoa(value, buff, 10);
  debugPrint(x, y, size, buff, 6);
}

void PHN_Display::debugPrint(uint16_t x, uint16_t y, uint8_t size, float value) {
  char buff[7];
  dtostrf(value, 4, 2, buff);
  debugPrint(x, y, size, buff, 6);
}

static float ImgMultColor_r, ImgMultColor_g, ImgMultColor_b;
static void ImgMultColor(uint8_t *r, uint8_t *g, uint8_t *b) {
  *r = (uint8_t) min((float) (*r) * ImgMultColor_r, 255);
  *g = (uint8_t) min((float) (*g) * ImgMultColor_g, 255);
  *b = (uint8_t) min((float) (*b) * ImgMultColor_b, 255);
}

void PHN_Display::drawImage(Stream &imageStream, int x, int y) {
  drawImageMain(imageStream, x, y, NULL, NULL);
}

void PHN_Display::drawImage(Stream &imageStream, int x, int y, float brightness) {
  drawImage(imageStream, x, y, brightness, brightness, brightness);
}

void PHN_Display::drawImage(Stream &imageStream, int x, int y, float cr, float cg, float cb) {
  // Set color component factors, then draw using the ImgMultColor color transform function
  ImgMultColor_r = cr; ImgMultColor_g = cg; ImgMultColor_b = cb;
  drawImageMain(imageStream, x, y, ImgMultColor, NULL);
}
void PHN_Display::drawImageColorFunc(Stream &imageStream, int x, int y, void (*color)(uint8_t*, uint8_t*, uint8_t*)) {
  drawImageMain(imageStream, x, y, color, NULL);
}

void PHN_Display::drawImageColorMap(Stream &imageStream, int x, int y, color_t *colorMapInput) {
  drawImageMain(imageStream, x, y, NULL, colorMapInput);
}

void PHN_Display::drawImageMain(Stream &imageStream, int x, int y, void (*color)(uint8_t*, uint8_t*, uint8_t*), color_t *colorMapInput) {
  // Store old viewport for later restoring
  Viewport oldViewport = getViewport();

  char idChar = imageStream.read();
  if (idChar == 'B') {
    if (imageStream.read() == 'M') {
      // Bitmap
      // read the BITMAP file header
      Imageheader_BMP header;
      imageStream.readBytes((char*) &header, sizeof(header));
      if (!header.biClrUsed) header.biClrUsed = (1 << header.bitCount);
  
      // Read the pixel map
      color_t colorMap[header.biClrUsed];
      Color_ARGB argb;
      uint16_t ci;
      for (ci = 0; ci < header.biClrUsed; ci++) {
        imageStream.readBytes((char*) &argb, sizeof(argb));
        if (colorMapInput) {
          colorMap[ci] = colorMapInput[ci];
        } else {
          if (color) {
            color(&argb.r, &argb.g, &argb.b);
          }
          colorMap[ci] = PHNDisplayHW::color565(argb.r, argb.g, argb.b);
        }
      }

      // Skip ahead towards the pixel map (this is usually never needed)
      uint32_t pos = sizeof(header) + 2 + (header.biClrUsed * 4);
      for (; pos < header.pixelDataOffset; pos++) {
        imageStream.read();
      }

      // Set up the viewport to render the bitmap in
      setViewportRelative(x, y, header.width, header.height);

      // Go to the bottom-left pixel, drawing UPWARDS (!)
      setWrapMode(WRAPMODE_UP);
      goTo(0, header.height - 1, 0);

      // Used variables and calculated constants
      uint32_t px, py;
      uint32_t imageWidthSize = ((uint32_t) header.bitCount * header.width) >> 3;
      uint8_t imageWidthPadding = (4 - (imageWidthSize & 0x3)) & 0x3;

      // Process each row of pixels
      for (py = 0; py < header.height; py++) {
        // Draw visible Bitmap portion
        if (header.bitCount == 8) {
          for (px = 0; px < header.width; px++) {
            PHNDisplay16Bit::writePixel(colorMap[imageStream.read() & 0xFF]);
          }
        } else if (header.bitCount == 24) {
          // Color transform used?
          for (px = 0; px < header.width; px++) {
            imageStream.readBytes((char*) &argb, 3);
            if (color) color(&argb.r, &argb.g, &argb.b);
            PHNDisplay16Bit::writePixel(PHNDisplayHW::color565(argb.r, argb.g, argb.b));
          }
        }

        // Handle padding
        for (px = 0; px < imageWidthPadding; px++) {
          imageStream.read();
        }
      }
    }
  } else if (idChar == 'L') { 
    if (imageStream.read() == 'C' && imageStream.read() == 'D') {
      // LCD format
      // Read LCD headers
      Imageheader_LCD header;
      imageStream.readBytes((char*) &header, sizeof(header));
      uint32_t imagePixels = header.width * header.height;

      // Read colormap, empty if not used/available
      color_t colorMap[header.colors];
      uint16_t ci;
      color_t c565;
      uint8_t r, g, b;
      for (ci = 0; ci < header.colors; ci++) {
        imageStream.readBytes((char*) &c565, sizeof(c565));
        if (colorMapInput) {
          colorMap[ci] = colorMapInput[ci];
        } else if (color) {
          r = PHNDisplayHW::color565Red(c565);
          g = PHNDisplayHW::color565Green(c565);
          b = PHNDisplayHW::color565Blue(c565);
          color(&r, &g, &b);
          colorMap[ci] = PHNDisplayHW::color565(r, g, b);
        } else {
          colorMap[ci] = c565;
        }
      }

      // Set up the viewport to render the image in
      setViewportRelative(x, y, header.width, header.height);

      // Go to the top-left pixel, drawing DOWNWARDS (!)
      setWrapMode(WRAPMODE_DOWN);
      goTo(0, 0, 0);

      // Write pixels to screen
      if (header.bpp == 16) {
        // Fast method
        uint32_t i;
        for (i = 0; i < imagePixels; i++) {
          imageStream.readBytes((char*) &c565, sizeof(c565));
          if (color) {
            r = PHNDisplayHW::color565Red(c565);
            g = PHNDisplayHW::color565Green(c565);
            b = PHNDisplayHW::color565Blue(c565);
            color(&r, &g, &b);
            c565 = PHNDisplayHW::color565(r, g, b);
          }
          PHNDisplay16Bit::writePixel(c565);
        }
      } else {
        int tmpbuff = 0;
        int tmpbuff_len = 0;
        int pixelmask = (1 << header.bpp) - 1;
        uint32_t i;
        for (i = 0; i < imagePixels; i++) {
          if (!tmpbuff_len) {
            tmpbuff = (imageStream.read() & 0xFF);
            tmpbuff_len = 8;
          }
          PHNDisplay16Bit::writePixel(colorMap[tmpbuff & pixelmask]);
          tmpbuff >>= header.bpp;
          tmpbuff_len -= header.bpp;
        }
      }
    }
  }
  // Restore viewport
  setViewport(oldViewport);
}