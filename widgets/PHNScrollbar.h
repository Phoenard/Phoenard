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
 * @brief Contains the PHN_Scrollbar widget; a widget for a bar that can be dragged
 */

#include "PHNWidget.h"

#ifndef _PHN_SCROLLBAR_H_
#define _PHN_SCROLLBAR_H_

/// Time it takes to press the incr/decr buttons before automatically scrolling
#define SCROLL_AUTO_DELAY 800
/// Time it takes to increment or decrement while in auto-scrolling mode
#define SCROLL_AUTO_TICK_DELAY 30

/**
 * @brief Shows a scroll bar the user can drag
 *
 * Depending on the width and height a vertical or horizontal bar is used.
 * Set up the range before use.
 */
class PHN_Scrollbar : public PHN_Widget {
 public:
  /// Initializes the value to the pure minimum
  PHN_Scrollbar(void) : scrollMin(0), scrollMax(0), scrollPos(0) {}
  /// Sets the minimum and maximum scrolling range
  void setRange(int minValue, int maxValue);
  /// Gets the range minimum value
  int minValue() const { return scrollMin; }
  /// Gets the range maximum value
  int maxValue() const { return scrollMax; }
  /// Sets the scrollbar value
  void setValue(int value);
  /// Gets the scrollbar value
  const int value(void) { return scrollPos; }
  /// Gets whether the value was changed since the last update
  bool isValueChanged(void) const { return valueChanged; }

  virtual void draw(void);
  virtual void update(void);
 private:
  void updateBar(bool redrawing);
  void drawArrow(int x, int y, int w, int h, int direction, bool highlight);
  int scrollPos, scrollMin, scrollMax;
  int sliderPos;
  char prevScroll;
  unsigned long scrollTime;
  bool barWasPressed;
  bool valueChanged;
};

#endif