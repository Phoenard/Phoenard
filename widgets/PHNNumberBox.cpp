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
  setRange(-0x7FFF, 0x7FFF);
  display.addWidget(scroll);
}

void PHN_NumberBox::setTextRaw(const char* text, int textLen) {
  textBuff.setTextRaw(text, textLen);
  textDirty = true;
}

void PHN_NumberBox::setValue(int value) {
  if (value == scroll.value()) {
    return;
  }
  scroll.setValue(value);
  updateText();
}

void PHN_NumberBox::updateText() {
  // Update the text silently without causing invalidation
  bool wasInvalidated = textDirty;
  setText(scroll.value());
  if (!wasInvalidated) {
    textDirty = false;
    valueChanged = true;
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
  } else if (valueChanged) {
    valueChanged = false;
    textDirty = true;
  }
  if (scroll.isValueChanged()) {
    updateText();
  }

  // Redraw text only
  if (textDirty) {
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
  display.setTextSize(bounds.size);
  display.setCursor(bounds.x, bounds.y);
  display.print(text());
}