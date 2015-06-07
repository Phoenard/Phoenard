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

#include "PHNKeyboard.h"

void PHN_Keyboard::setDimension(int rows, int columns) {
  this->rows = rows;
  this->cols = columns;
  this->count = (int) rows * (int) columns;
  setupCells();
}

void PHN_Keyboard::setSpacing(int spacing) {
  this->spacW = spacing;
  this->spacH = spacing;
  setupCells();
}

void PHN_Keyboard::setSpacing(int spacingW, int spacingH) {
  this->spacW = spacingW;
  this->spacH = spacingH;
  setupCells();
}

void PHN_Keyboard::setKeys(const char* keyChars) {
  this->keyChars.set(keyChars, strlen(keyChars));
  invalidate();
}

void PHN_Keyboard::setupCells() {
  // Calculate the cell width and height using the bounds and spacing
  cellW = (width  - (spacW * (cols - 1))) / cols;
  cellH = (height - (spacH * (rows - 1))) / rows;
  // Force a redraw
  invalidate();
}

char PHN_Keyboard::clickedKey() {
   if (this->clickedIdx == -1) {
     return 0;
   } else {
     return key(this->clickedIdx);
   }
}

void PHN_Keyboard::update() {
  int oldPressedIdx = pressedIdx;
  
  pressedIdx = -1;
  for (int i = 0; i < count; i++) {
    updateCell(i, false);
  }
  if (pressedIdx != oldPressedIdx) {
    updateCell(oldPressedIdx, true);
  }
  clickedIdx = -1;
  if (oldPressedIdx != -1 && !display.isTouched()) {
    clickedIdx = oldPressedIdx;
  }
}

void PHN_Keyboard::draw() {
  // Fill with background color
  fillWidgetArea(color(BACKGROUND));

  // Go by all the cells, force-updating them
  clickedIdx = -1;
  for (int i = 0; i < count; i++) {
    updateCell(i, true);
  }
}

void PHN_Keyboard::updateCell(int idx, bool forceDraw) {
  // Ignore these
  if (idx == -1) return;
  
  // Get the cell coordinates and other information
  int row = idx / cols;
  int col = idx % cols;
  int cy = this->y + row * (cellH + spacH);
  int cx = this->x + col * (cellW + spacW);
  int cw = cellW;
  char txt[2] = {key(idx), 0};

  // If text is the same as the character before, it is one key
  // In that case, don't do anything here
  if (col > 0 && key(idx-1) == txt[0]) return;
  
  // Get the amount of cells covered by the same character
  for (unsigned char i = 1; i < (cols-col); i++) {
    if (key(idx+i) != txt[0]) break;
    cw += cellW + spacW;
  }

  // Update as needed
  bool isTouched = display.isTouched(cx, cy, cw, cellH);
  if ((isTouched != (idx == pressedIdx)) || forceDraw) {
    color_t key_color = color(isTouched ? HIGHLIGHT : FOREGROUND);
    display.fillRect(cx, cy, cw, cellH, key_color);
    display.drawRect(cx, cy, cw, cellH, color(FRAME));
    display.setTextColor(color(CONTENT), key_color);
    display.drawStringMiddle(cx, cy, cw, cellH, txt);
  }
  if (isTouched) pressedIdx = idx;
}