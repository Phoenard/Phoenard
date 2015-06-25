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
  bool longLayout = (width > height);
  bool scrollReversed = (scrollMax < scrollMin);
  int barSize = longLayout ? (width-height*2) : (height-width*2);
  int scrollStart = min(scrollMin, scrollMax);
  int scrollDiff = abs(scrollMax-scrollMin);
  int barHandleSize = max(3, (scrollDiff==1) ? (barSize/2) : (barSize / scrollDiff));
  bool barIsTouched = false;
  bool incrIsTouched = false;
  bool decrIsTouched = false;
  bool navIsPressed = false;
  bool barPressChanged = false;
  bool barChanged = valueChanged;

  // Reset value changed to false
  valueChanged = false;

  // Update touch input
  barIsTouched = display.isTouched(x, y, width, height);
  barPressChanged = (barWasPressed != barIsTouched);
  barWasPressed = barIsTouched;
  if (barIsTouched) {
    float fact;
    if (longLayout) {
      int xPos_a = x+height;
      int xPos_b = x+width-height-barHandleSize;
      fact = (float) (display.getTouch().x-xPos_a) / (float) (xPos_b-xPos_a);
    } else {
      int yPos_a = y+width;
      int yPos_b = y+height-width-barHandleSize;
      fact = (float) (display.getTouch().y-yPos_a) / (float) (yPos_b-yPos_a);
      fact = 1.0F - fact;
    }
    if (scrollReversed) fact = 1.0F - fact;
    decrIsTouched = (fact < 0.0F);
    incrIsTouched = (fact > 1.0F);
    navIsPressed = decrIsTouched || incrIsTouched;
    if (!navIsPressed) {
      setValue(scrollStart + (int)(scrollDiff * fact));
      barChanged |= valueChanged;
    }
  }

  // Refresh the scroll increment buttons
  if (barPressChanged) {
    if (incrIsTouched) setValue(value() + 1);
    if (decrIsTouched) setValue(value() - 1);
  }

  // Draw border between buttons and bar
  if (redrawing) {
    if (longLayout) {
      display.drawVerticalLine(x+height-1, y, height-1, color(FRAME));
      display.drawVerticalLine(x+width-height, y, height-1, color(FRAME));
    } else {
      display.drawHorizontalLine(x+1, y+width-1, width-1, color(FRAME));
      display.drawHorizontalLine(x+1, y+height-width, width-1, color(FRAME));
    }
  }

  // When no scrolling is possible, abort at this point
  // This shows a 'disabled' state where no arrows/bar are visible
  if (scrollMin == scrollMax) {
    return;
  }

  // Draw the frame
  if (redrawing || barPressChanged || (navIsPressed != navWasPressed)) {
    navWasPressed = navIsPressed;
    color_t color_btn1 = color(incrIsTouched ? HIGHLIGHT : FOREGROUND);
    color_t color_btn2 = color(decrIsTouched ? HIGHLIGHT : FOREGROUND);
    if (longLayout != scrollReversed) {
      color_t c = color_btn2;
      color_btn2 = color_btn1;
      color_btn1 = c;
    }
    if (longLayout) {    
      // Draw the left/right buttons
      display.fillRect(x+1, y+1, height-2, height-2, color_btn1);
      display.fillRect(x+width-height+1, y+1, height-2, height-2, color_btn2);

      // Draw the left/right arrows
      int tri_size = (int) (height*0.7);
      int tri_start = (height>>1) - (tri_size>>1);
      display.drawTriangle(x+tri_start-1, y+tri_start+(tri_size>>1),
                           x+tri_start+tri_size-1, y+height-tri_start,
                           x+tri_start+tri_size-1, y+tri_start, color(CONTENT));
      display.drawTriangle(x+width-tri_start, y+tri_start+(tri_size>>1),
                           x+width-tri_size-tri_start, y+tri_start,
                           x+width-tri_size-tri_start, y+height-tri_start, color(CONTENT));
    } else {
      // Draw the up/down buttons
      display.fillRect(x+1, y+1, width-2, width-2, color_btn1);
      display.fillRect(x+1, y+height-width+1, width-2, width-2, color_btn2);

      // Draw the up/down arrows
      int tri_size = (int) (width*0.7);
      int tri_start = (width>>1) - (tri_size>>1);
      display.drawTriangle(x+tri_start, y+tri_start+tri_size-1,
                           x+tri_start+tri_size, y+tri_start+tri_size-1,
                           x+tri_start+(tri_size>>1), y+tri_start-1, color(CONTENT));
      display.drawTriangle(x+tri_start, y+height-tri_start-tri_size,
                           x+tri_start+tri_size, y+height-tri_start-tri_size,
                           x+tri_start+(tri_size>>1), y+height-tri_start, color(CONTENT));
    }
  }

  // Draw the bar when forced and only when the bar is larger than 10
  if ((redrawing || barChanged || barPressChanged) && (barSize > 10)) {
    int scrollPosFixed = scrollReversed ? (scrollMin - scrollPos) : scrollPos;
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