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

#include "PHNLabel.h"

PHN_Label::PHN_Label() {
  setColor(CONTENT, color(FOREGROUND));
  drawFrame = true;
}

void PHN_Label::setText(const char* text) {
  this->textBuff.set(text, strlen(text) + 1);
  invalidate();
}

void PHN_Label::setText(String &text) {
  this->textBuff.resize(text.length() + 1);
  text.toCharArray((char*) this->textBuff.data, this->textBuff.dataSize);
  invalidate();
}

void PHN_Label::update() {
}

void PHN_Label::draw() {
  // Draw the background shape
  display.fillRect(x, y, width, height, color(BACKGROUND));
  if (drawFrame) {
    display.drawRect(x, y, width, height, color(FRAME));
  }
  
  // Draw the text
  display.setTextColor(color(CONTENT));
  display.drawStringMiddle(x+1, y+1, width-2, height-2, (char*) textBuff.data);
}