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

#include "PHNScrollbar.h"

void PHN_Scrollbar::setRange(int minValue, int maxValue) {
  if (scrollMin != minValue || scrollMax != maxValue) {
    scrollMin = minValue;
    scrollMax = maxValue;
    setValue(value());
    invalidate();
  }
}

void PHN_Scrollbar::setValue(int value) {
  int newValue;
  if (value < scrollMin) {
    newValue = scrollMin;
  } else if (value > scrollMax) {
    newValue = scrollMax;
  } else {
    newValue = value;
  }
  if (newValue != scrollPos) {
    valueChanged = true;
    scrollPos = newValue;
    invalidate();
  }
}

void PHN_Scrollbar::draw() {
  if (!width || !height) {
    return;
  }
  display.fillRect(x+1, y+1, width-2, height-2, color(FOREGROUND));
  display.drawRect(x, y, width, height, color(FRAME));
  
  // Draw the lines distinguishing the up/down scroll buttons
  if (longLayout) {
    display.drawVerticalLine(x+height-1, y, height-1, color(FRAME));
    display.drawVerticalLine(x+width-height, y, height-1, color(FRAME));
  } else {
    display.drawHorizontalLine(x+1, y+width-1, width-1, color(FRAME));
    display.drawHorizontalLine(x+1, y+height-width, width-1, color(FRAME));
  }
  // When no scrolling is possible, abort at this point
  // This shows a 'disabled' state where no arrows/bar are visible
  if (scrollMin == scrollMax) {
    return;
  }

  if (longLayout) {
    if (pressedUp) {
      display.fillRect(x+1, y+1, height-2, height-2, color(HIGHLIGHT));
    }
    if (pressedDown) {
      display.fillRect(x+width-height, y+1, height-2, height-2, color(HIGHLIGHT));
    }

    // Draw up/down arrows
    int tri_size = (int) (height*0.7);
    int tri_start = (height>>1) - (tri_size>>1);
    display.drawTriangle(x+tri_start-1, y+height-tri_start,
                         x+tri_start+tri_size-1, y+height-tri_start,
                         x+tri_start+(tri_size>>1), y+tri_start, color(CONTENT));
    display.drawTriangle(x+width-tri_start-tri_size, y+tri_start,
                         x+width-tri_start, y+tri_start,
                         x+width-tri_start-(tri_size>>1)-1, y+height-tri_start, color(CONTENT));
    
    // No scrollbar drawn when in minimal long-layout
  } else {
    if (pressedUp) {
      display.fillRect(x+1, y+1, width-2, width-2, color(HIGHLIGHT));
    }
    if (pressedDown) {
      display.fillRect(x+1, y+height-width+1, width-2, width-2, color(HIGHLIGHT));
    }

    // Draw up/down arrows
    int tri_size = (int) (width*0.7);
    int tri_start = (width>>1) - (tri_size>>1);
    display.drawTriangle(x+tri_start, y+tri_start+tri_size-1,
                         x+tri_start+tri_size, y+tri_start+tri_size-1,
                         x+tri_start+(tri_size>>1), y+tri_start-1, color(CONTENT));
    display.drawTriangle(x+tri_start, y+height-tri_start-tri_size,
                         x+tri_start+tri_size, y+height-tri_start-tri_size,
                         x+tri_start+(tri_size>>1), y+height-tri_start, color(CONTENT));

    // Draw the bar area
    if (barSize > 10) {
      int scrollDiff = (scrollMax-scrollMin);
      int barHandleSize = max(3, (scrollDiff==1) ? (barSize/2) : (barSize / scrollDiff));
      int barOffset = (int) ((float) (barSize-barHandleSize) * (float) (scrollPos-scrollMin) / (float)scrollDiff);
      display.drawRect(x+1, y+width+barOffset, width-2, barHandleSize, color(CONTENT));
      if (pressedBar) {
        display.fillRect(x+2, y+width+barOffset+1, width-4, barHandleSize-2, color(HIGHLIGHT));
      }
    }
  }           
}

void PHN_Scrollbar::update() {
  this->longLayout = (width > height);
  this->barSize = height-width*2;
  this->valueChanged = false;

  // Read presses
  bool newUp, newDown, newBar;
  if (display.isTouched(x, y, width, height)) {
    if (longLayout) {
      newUp = display.isTouched(x, y, height, height);
      newDown = display.isTouched(x+width-height, y, height, height);
    } else {
      newUp = display.isTouched(x, y, width, width);
      newDown = display.isTouched(x, y+height-width, width, width);
    }
    // If not pressing the buttons, we are selecting somewhere in between
    newBar = !newUp && !newDown && !longLayout && barSize>10;
    if (newBar) {
      int scrollDiff = (scrollMax-scrollMin);
      int barHandleSize = max(3, (scrollDiff==1) ? (barSize/2) : (barSize / scrollDiff));
      int yPos_a = y+width;
      int yPos_b = y+height-width-barHandleSize;
      float fact = (float) (display.getTouch().y-yPos_a) / (float) (yPos_b-yPos_a);
      fact = constrain(fact, 0.0F, 1.0F);
      setValue(scrollMin + (int)(scrollDiff * fact));
    }
  } else {
    newUp = false;
    newDown = false;
    newBar = false;
  }

  // Update state
  if (newBar != pressedBar) {
    pressedBar = newBar;
    invalidate();
  }
  if (newUp != pressedUp) {
    pressedUp = newUp;
    invalidate();
    if (pressedUp)
      setValue(value() - 1);
  }
  if (newDown != pressedDown) {
    pressedDown = newDown;
    invalidate();
    if (pressedDown)
      setValue(value() + 1);
  }
}