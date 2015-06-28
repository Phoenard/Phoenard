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
 * @brief Contains the PHN_NumberBox widget; a textbox with value up/down buttons
 */

#include "PHNWidget.h"
#include "PHNScrollbar.h"

#ifndef _PHN_NUMBERBOX_H_
#define _PHN_NUMBERBOX_H_

/**
 * @brief The numberbox widget can be scrolled by the user to change between integer values
 *
 * The widget displays a rectangular box in which the current value or user-specified text
 * is displayed. To the right of the box are two buttons allowing the user to increment or
 * decrement the current value by 1.
 *
 * To display text selection instead of a number, the implementer can override the current
 * text after the value changes.
 *
 * When using the widget, make sure to specify the bounds and value range.
 */
class PHN_NumberBox : public PHN_Widget, public PHN_TextContainer {
public:
  /// Initializer
  PHN_NumberBox();
  /// Sets the minimum and maximum range of the value
  void setRange(int minValue, int maxValue);
  /// Gets the range minimum value
  int minValue() const { return _minValue; }
  /// Gets the range maximum value
  int maxValue() const { return _maxValue; }
  /// Increments or decrements the value
  void addValue(int increment);
  /// Sets the current value
  void setValue(int value);
  /// Gets the current value
  int value() { return _value; }
  /// Gets whether the value was changed
  bool isValueChanged() const { return _valueChanged; }
  /// Sets whether the value wraps around
  void setWrapAround(bool wrapAround);
  /// Gets last wrap-around increment performed

  /**@brief Gets the last wrap-around increment performed
   * 
   * When wrap-around is enabled, this value can be used to track
   * the event of wrapping around.
   *
   * This function returns:
   * +1 When the value is incremented and wrapped past the maximum
   * -1 When the value is decremented and wrapped past the minimum
   *  0 When no wrap-around events occurred
   */
  char wrapAroundIncrement() const { return _lastWrapAround; }

  virtual void setTextRaw(const char* text, int textLen);
  virtual const char* text() { return textBuff.text(); }
  virtual void draw(void);
  virtual void update(void);

private:
  TextBounds getTextBounds();
  void drawText(TextBounds bounds);

  int _minValue, _maxValue, _value;
  bool _valueChanged;
  char _lastWrapAround;
  int scrollWidth;
  bool wrapAround;
  bool textDirty;
  TextBounds lastTextBounds;
  DataCopyBuffer textBuff;
  PHN_Scrollbar scroll;
};

#endif