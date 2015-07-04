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
  if ((scrollMin != minValue) || (scrollMax != maxValue)) {
    scrollMin = minValue;
    scrollMax = maxValue;
    invalidate();
  }
  setValue(value());
  valueChanged = true;
}

void PHN_Scrollbar::setValue(int value) {
  int newValue;
  int minVal = min(scrollMin, scrollMax);
  int maxVal = max(scrollMin, scrollMax);
  if (value < minVal) {
    newValue = minVal;
  } else if (value > maxVal) {
    newValue = maxVal;
  } else {
    newValue = value;
  }
  if (newValue != scrollPos) {
    valueChanged = true;
    scrollPos = newValue;
  }
}

void PHN_Scrollbar::draw() {
  // Draw background/frame
  display.fillRect(x+1, y+1, width-2, height-2, color(FOREGROUND));
  display.drawRect(x, y, width, height, color(FRAME));

  // Draw the rest of the bar
  updateBar(true);
}

void PHN_Scrollbar::update() {
  if (!invalidated) updateBar(false);
}

void PHN_Scrollbar::updateBar(bool redrawing) {
  // Scrollbar parameters for long (horizontal) and vertical layout
  bool longLayout = (width > height);
  int btnWidth = longLayout ? width : height;
  int btnHeight = longLayout ? height : width;
  int barSize = (btnWidth-btnHeight*2);

  // Scroll limits and scroll reverse state from scroll range
  bool scrollReversed = (scrollMax < scrollMin);
  int scrollStart = scrollReversed ? scrollMax : scrollMin;
  int scrollDiff = scrollReversed ? scrollMin : scrollMax;
  char scrollIncr = 0;
  bool scrollVisible = (barSize > 10);
  int barHandleSize = (barSize / (scrollDiff+1));
  bool barIsTouched = false;
  bool barPressChanged = false;
  bool barChanged = valueChanged;

  // Make sure bar handle is kept >= 3
  if (barHandleSize < 3) barHandleSize = 3;

  // Reset value changed to false
  valueChanged = false;

  // Half-size for scroll buttons that are tiny
  if (barSize <= 0) btnHeight >>= 1;

  // If we touched the scroll buttons earlier, increase touch bounds
  // This makes it easier to control small bars
  int tgrow = (prevScroll == 0) ? 0 : 5;
  
  // Update touch input
  barIsTouched = display.isTouched(x-tgrow, y-tgrow, width+2*tgrow, height+2*tgrow);
  barPressChanged = (barWasPressed != barIsTouched);
  barWasPressed = barIsTouched;
  if (barIsTouched) {
    int pos_a, pos_b, pos_v;
    pos_a = btnHeight;
    pos_b = btnWidth-btnHeight;
    if (longLayout) {
      pos_v = display.getTouch().x-x;
    } else {
      pos_v = display.getTouch().y-y;
    }
    if (longLayout == scrollReversed) {
      pos_v = pos_a+pos_b-pos_v;
    }
    if ((prevScroll > 0) && ((pos_v-10) < pos_a)) {
      // Allow a little more range here
      scrollIncr = 1;
    } else if ((prevScroll < 0) && ((pos_v+10) > pos_b)) {
      // Allow a little more range here
      scrollIncr = -1;
    } else if (pos_v < pos_a) {
      scrollIncr = -1;
    } else if (pos_v > pos_b) {
      scrollIncr = 1;
    }
    if ((scrollIncr==0) && scrollVisible) {
      float fact = (float) (pos_v-pos_a) / (float) (pos_b-pos_a-barHandleSize);
      setValue(scrollStart + (int) (scrollDiff * fact));
      barChanged |= valueChanged;
    }
  }

  // Reset scroll timer while buttons are not pressed down
  unsigned long time = millis();
  bool scrollChanged = (scrollIncr != prevScroll);
  if (scrollIncr == 0 || scrollChanged) scrollTime = time;

  // Check if auto-scrolling is active
  bool autoScroll = false;
  if ((time - scrollTime) >= SCROLL_AUTO_DELAY) {
    scrollTime = time - SCROLL_AUTO_DELAY + SCROLL_AUTO_TICK_DELAY;
    autoScroll = true;
  }

  // Perform incrementing here
  if (scrollChanged || autoScroll) {
    setValue(value() + scrollIncr);
  }

  // Draw the arrow key buttons
  if (redrawing || barPressChanged || scrollChanged) {
    prevScroll = scrollIncr;
    bool btn1_high = scrollIncr>0;
    bool btn2_high = scrollIncr<0;
    if (longLayout != scrollReversed) {
      btn1_high = scrollIncr<0;
      btn2_high = scrollIncr>0;
    }
    if (longLayout) {
      // Draw the left/right buttons
      drawArrow(x, y, btnHeight, height, 0, btn1_high);
      drawArrow(x+width-btnHeight, y, btnHeight, height, 1, btn2_high);
    } else {
      // Draw the up/down buttons
      drawArrow(x, y, width, btnHeight, 2, btn1_high);
      drawArrow(x, y+height-btnHeight, width, btnHeight, 3, btn2_high);
    }
  }

  // When no scrolling is possible, abort at this point
  // This shows a 'disabled' state where no arrows/bar are visible
  if (scrollMin == scrollMax) {
    return;
  }

  // Draw the bar when forced and only when the bar is visible
  if ((redrawing || barChanged || barPressChanged) && scrollVisible) {
    int scrollPosFixed = scrollReversed ? ((scrollMin+scrollMax)-scrollPos) : scrollPos;
    int prevSliderPos = this->sliderPos;
    this->sliderPos = (int) ((float) (barSize-barHandleSize) * (float) (scrollPosFixed-scrollStart) / (float)scrollDiff);
    if (longLayout) {
      // Transform position for horizontal layout
      this->sliderPos = x+height+this->sliderPos;

      // Erase previous bar if not redrawing / first draw
      if (!redrawing) {
        display.fillRect(prevSliderPos, y+1, barHandleSize, height-2, color(FOREGROUND));
      }

      // Update and draw the new bar
      display.drawRect(this->sliderPos, y+1, barHandleSize, height-2, color(CONTENT));
      display.fillRect(this->sliderPos+1, y+2, barHandleSize-2, height-4, color(barIsTouched ? HIGHLIGHT : FOREGROUND));
    } else {
      // Transform position for vertical layout
      this->sliderPos = y+height-width-this->sliderPos-barHandleSize;

      // Erase previous bar if not redrawing / first draw
      if (!redrawing) {
        display.fillRect(x+1, prevSliderPos, width-2, barHandleSize, color(FOREGROUND));
      }

      // Update and draw the new bar
      display.drawRect(x+1, this->sliderPos, width-2, barHandleSize, color(CONTENT));
      display.fillRect(x+2, this->sliderPos+1, width-4, barHandleSize-2, color(barIsTouched ? HIGHLIGHT : FOREGROUND));
    }
  }
}

void PHN_Scrollbar::drawArrow(int x, int y, int w, int h, int direction, bool highlight) {
  int tri_w = (2*w/3);
  int tri_h = (2*h/3);
  int p[6];
  p[0] = p[2] = p[4] = x + ((w-tri_w)>>1);
  p[1] = p[3] = p[5] = y + ((h-tri_h)>>1);
  if (direction & 0x2) {
    // Up/Down
    p[0] += (tri_w>>1);
    p[4] += tri_w;
    if (direction == 2) {
      // Up
      p[3] += tri_h;
      p[5] += tri_h;
    } else {
      // Down
      p[1] += tri_h;
    }
  } else {
    // Left/Right
    p[1] += (tri_h>>1);
    p[3] += tri_h;
    if (direction == 0) {
      // Left
      p[4] += tri_w;
      p[2] += tri_w;
    } else {
      // Right
      p[0] += tri_w;
    }
  }
  display.fillBorderRect(x, y, w, h, color(highlight ? HIGHLIGHT : FOREGROUND), color(FRAME));
  display.drawTriangle(p[0], p[1], p[2], p[3], p[4], p[5], color((scrollMin == scrollMax) ? FOREGROUND : CONTENT));
}