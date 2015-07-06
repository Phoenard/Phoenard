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

#include "PHNNumberBox.h"

PHN_NumberBox::PHN_NumberBox() {
  wrapAround = false;
  _valueChanged = true;
  _firstUpdate = true;
  setRange(-0x7FFE, 0x7FFE);
  addWidget(scroll);
}

void PHN_NumberBox::setTextRaw(const char* text, int textLen) {
  textBuff.setTextRaw(text, textLen);
  textDirty = true;
}

void PHN_NumberBox::addValue(int increment) {
  setValue(value() + increment);
}

void PHN_NumberBox::setValue(int value) {
  // Keep value within bounds, or perform wrap-around logic
  if (value > _maxValue) {
    if (wrapAround) {
      _lastWrapAround = 1;
      value = _minValue + (value - _maxValue) - 1;
    } else {
      value = _maxValue;
    }
  } else if (value < _minValue) {
    if (wrapAround) {
      _lastWrapAround = -1;
      value = _maxValue + (value - _minValue) + 1;
    } else {
      value = _minValue;
    }
  }
  scroll.setValue(value);
  if (scroll.isValueChanged()) {
    // Update text when the value changes
    // Update the text silently without causing redrawing
    textBuff.setText(value);

    // Notify changed - causes a redraw next update
    _valueChanged = true;
  }
}

void PHN_NumberBox::setRange(int minValue, int maxValue) {
  if (_minValue == minValue && _maxValue == maxValue) {
    return;
  }
  _minValue = minValue;
  _maxValue = maxValue;
  setValue(constrain(value(), minValue, maxValue));
  scroll.setRange(minValue-1, maxValue+1);
}

void PHN_NumberBox::setWrapAround(bool wrapAround) {
  if (this->wrapAround != wrapAround) {
    this->wrapAround = wrapAround;
    setRange(_minValue, _maxValue);
  }
}

TextBounds PHN_NumberBox::getTextBounds() {
  return display.computeMiddleBounds(x+1, y+1, width-scrollWidth-2, height-2, text());
}

void PHN_NumberBox::update() {
  if (invalidated) {
    textDirty = false;
    scrollWidth = height;
    scroll.setBounds(x+width-scrollWidth, y, scrollWidth, height);
  } else if (_valueChanged) {
    // If value was changed, redraw the text next
    textDirty = true;
  }

  // Reset state
  _valueChanged = false;
  _lastWrapAround = 0;

  // Update input from the scrollbar
  // This makes sure to detect changes
  // As well, it handles wrap-around logic
  setValue(scroll.value());

  // Value is always changed during the first update
  if (_firstUpdate) {
    _valueChanged = true;
  }

  // Redraw text only
  if (!invalidated && textDirty) {
    TextBounds bounds = getTextBounds();
    if (lastTextBounds == bounds) {
      drawText(bounds);
    } else {
      invalidate();
    }
  }
}

void PHN_NumberBox::draw() {
  display.fillBorderRect(x, y, width-scrollWidth+1, height, color(FOREGROUND), color(FRAME));
  drawText(getTextBounds());
}

void PHN_NumberBox::drawText(TextBounds bounds) {
  this->lastTextBounds = bounds;
  this->textDirty = false;
  display.setTextColor(color(CONTENT), color(FOREGROUND));

  // Do not draw anything during the first update
  // Reason for this is that the widget might be
  // meant to display something in place of numbers
  // This requires at least one full update to be done.
  if (_firstUpdate) {
    _firstUpdate = false;
  } else {
    display.drawString(bounds.x, bounds.y, text(), bounds.size);
  }
}