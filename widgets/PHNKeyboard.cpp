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

// Maximum length you can use as a name for a keyboard format
const int MAX_FORMAT_NAME_LENGTH = 15;
// Character used to indicate the 'change format' key
const char CHANGE_FMT_KEY = '\f';

PHN_Keyboard::PHN_Keyboard() {
  this->formatIdx = 0;
  this->formatCnt = 0;
}

void PHN_Keyboard::setDimension(int columns, int rows) {
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
  clearKeys();
  addKeys(keyChars);
}

void PHN_Keyboard::clearKeys() {
  // Erase all current keys
  for (int i = 0; i < this->count; i++) {
    this->updateCell(i, false, true);
  }

  // Reset
  this->formatIdx = 0;
  this->formatCnt = 0;
  this->formatChars.resize(0);
}

void PHN_Keyboard::addKeys(const char* keyChars) {
  addKeys("", keyChars);
}

void PHN_Keyboard::addKeys(const char* formatName, const char* keyChars) {
  // Add keys to format mapping
  int txtLen = (this->count+1);
  int charsLen = strlen(keyChars);
  char* charBuff;
  this->formatCnt++;
  this->formatChars.resize(this->formatCnt * txtLen);
  charBuff = this->formatChars.text()+(this->formatCnt-1)*txtLen;
  memcpy(charBuff, keyChars, txtLen);

  // Pad end of the String with '\r' characters to show an empty space there
  if (charsLen < txtLen) {
    memset(charBuff+charsLen, '\r', txtLen-charsLen-1);
  }

  // Make sure to null-terminate!
  charBuff[txtLen-1] = 0;

  // Add format name to format name mapping
  const char fmtLen = (MAX_FORMAT_NAME_LENGTH+1);
  char fmtName[fmtLen];
  memcpy(fmtName, formatName, fmtLen-1);
  fmtName[fmtLen-1] = 0;
  this->formatNames.resize(this->formatCnt * fmtLen);
  memcpy(this->formatNames.text() + (this->formatCnt-1)*fmtLen, formatName, fmtLen);

  // Redraw later
  invalidate();
}

void PHN_Keyboard::nextFormat() {
  setFormatIndex((this->formatIdx + 1) % this->formatCnt);
}

void PHN_Keyboard::setFormatIndex(int index) {
  if (this->formatIdx != index) {
    // Some format changes require the previous format to be partially erased
    // This prevents glitched keys being displayed
    for (int i = 0; i < this->count; i++) {
      char key_old = fmt_key(this->formatIdx, i);
      char key_old_next = fmt_key(this->formatIdx, i+1);
      char key_new = fmt_key(index, i);
      char key_new_next = fmt_key(index, i+1);
      
      // Change from non-empty to empty requires erasing
      if ((key_old != key_new) && (key_new == '\r')) {
        updateCell(i, true, true);
      }

      // Change from double-key to single-key requires erasing
      if ((key_old==key_old_next) && (key_new!=key_new_next)) {
        updateCell(i, true, true);
        updateCell(i+1, true, true);
      }
    }

    // Update and redraw the keys
    this->formatIdx = index;
    invalidate();
  }
}

void PHN_Keyboard::setupCells() {
  // Calculate the cell width and height using the bounds and spacing
  cellW = (width  - (spacW * (cols - 1))) / cols;
  cellH = (height - (spacH * (rows - 1))) / rows;
  // Force a redraw
  invalidate();
}

char PHN_Keyboard::fmt_key(int fmtIndex, int index) {
  const char* chars = this->formatChars.text() + (fmtIndex*(this->count+1));
  return chars[index];
}

char PHN_Keyboard::key(int index) {
  return fmt_key(this->formatIdx, index);
}

char PHN_Keyboard::clickedKey() {
   char c = (this->clickedIdx == -1) ? 0 : key(this->clickedIdx);
   if (c == CHANGE_FMT_KEY) c = 0;;
   return c;
}

void PHN_Keyboard::update() {
  // Don't do anything now when invalidated
  if (invalidated) return;

  // Update key input
  int oldPressedIdx = pressedIdx;
  pressedIdx = -1;
  for (int i = 0; i < count; i++) {
    updateCell(i, false, false);
  }
  if (pressedIdx != oldPressedIdx) {
    updateCell(pressedIdx, true, false);
    updateCell(oldPressedIdx, true, false);
  }
  clickedIdx = -1;
  if (oldPressedIdx != -1 && !display.isTouched()) {
    clickedIdx = oldPressedIdx;
    
    // When clicking the format button, go to the next format
    // Only do this if multiple formats exist
    if ((key(this->clickedIdx) == CHANGE_FMT_KEY) && (this->formatCnt > 1)) {
      nextFormat();
    }
  }
}

void PHN_Keyboard::draw() {
  pressedIdx = -1;
  clickedIdx = -1;
  for (int i = 0; i < count; i++) {
    updateCell(i, true, false);
  }
}

void PHN_Keyboard::updateCell(int idx, bool drawCell, bool eraseCell) {
  // Ignore these
  if (idx == -1 || idx >= count) return;

  // Get the cell coordinates and other information
  int row = idx / cols;
  int col = idx % cols;
  int cy = this->y + row * (cellH + spacH);
  int cx = this->x + col * (cellW + spacW);
  int cw = cellW;
  int ch = cellH;
  char txt[2] = {key(idx), 0};

  // If text is the same as the character before, it is one key
  // In that case, don't do anything here
  if (col > 0 && key(idx-1) == txt[0]) return;

  // Get the amount of cells covered by the same character
  for (unsigned char i = 1; i < (cols-col); i++) {
    if (key(idx+i) != txt[0]) break;
    cw += cellW + spacW;
  }
  
  // Compute the key round rectangle radius to use from the cell width/height
  int rect_rad = 2 * min(cellW/5, cellH/8);

  // Update as needed
  bool isTouched = display.isTouched(cx, cy, cw, ch);
  if (drawCell) {
    if (txt[0] == '\r') {
      // Transparent 'open space' character
    } else if (eraseCell) {
      // Draw a black rectangle over it to erase the key
      display.fillRect(cx, cy, cw, ch, color(BACKGROUND));
    } else {
      color_t key_color = color(isTouched ? HIGHLIGHT : FOREGROUND);
      display.fillRoundRect(cx, cy, cw, ch, rect_rad, key_color);
      display.drawRoundRect(cx, cy, cw, ch, rect_rad, color(FRAME));
      display.setTextColor(color(CONTENT), key_color);

      // Find the text to display here
      const char* displayedText;
      if (txt[0] == '\t') {
        // Tab character, show 'TAB'
        displayedText = "TAB";
      } else if (txt[0] == '\n') {
        // Newline character, show 'ENTER'
        displayedText = (cw > cellW) ? "^ENTER" : "^";
      } else if (txt[0] == CHANGE_FMT_KEY) {
        // This character is used to show the next format name text
        // When this is pressed, the next format is selected
        int txtIdx = ((this->formatIdx+1)%this->formatCnt)*(MAX_FORMAT_NAME_LENGTH+1);
        displayedText = this->formatNames.text() + txtIdx;
      } else {
        // Regular ASCII character
        displayedText = txt;
      }

      // Find the bounds for displaying this text
      TextBounds bounds = display.computeMiddleBounds(cx, cy, cw, ch, displayedText);

      // For the newline key, we need to either show an arrow or arrow with text
      // The arrow (\n) can not be displayed with the standard print function
      if (txt[0] == '\n') {
        while (*displayedText) {
          char c = *(displayedText++);
          if (c == '^') c = '\n';
          display.drawChar(bounds.x, bounds.y, c, bounds.size);
          bounds.x += 6*bounds.size;
        }
      } else {
        // Draw it
        display.setTextSize(bounds.size);
        display.setCursor(bounds.x, bounds.y);
        display.print(displayedText);
      }
    }
  }
  if (isTouched) pressedIdx = idx;
}