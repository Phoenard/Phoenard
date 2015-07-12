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

#include "PHNLineGraph.h"

void PHN_LineGraph::setLineCount(int nrLines) {
  _lastValues.resize(nrLines * sizeof(unsigned int));
  _dim = nrLines;
}

void PHN_LineGraph::setRange(float minimum, float maximum) {
  _min = minimum;
  _max = maximum;
}

void PHN_LineGraph::clear() {
  _pos = 0;
  draw();
}

void PHN_LineGraph::addValue(float value) {
  float v[1];
  v[0] = value;
  addValues(v);
}

void PHN_LineGraph::addValues(const float* values) {
  // Clear screen as needed
  if (_pos >= width-2) {
    _pos = 0;
    if (!autoClearDisabled) {
      clear();
    }
  }

  // If not auto-clearing, wipe the next column first
  // Also clear the column when drawing for the first time
  if (autoClearDisabled || !isDrawn()) {
    display.drawVerticalLine(x+_pos+1, y+1, height-2, this->color(BACKGROUND));
  }

  // Draw the line
  for (unsigned char i = 0; i < _dim; i++) {
    color_t color = _lineColors.get(i);
    unsigned int *y_old_ptr = (unsigned int*) _lastValues.data + i;
    unsigned int x_new = _pos + 1;
    unsigned int y_new;
    if (values[i] <= _min) {
      y_new = 1;
    } else if (values[i] >= _max) {
      y_new = height-2;
    } else {
      y_new = 1 + (values[i] - _min) / (_max - _min) * (height-2);
    }

    // Reverse the y-value
    y_new = height - y_new - 1;

    // Use same old and new value if first value of graph
    if (_pos == 0) {
      *y_old_ptr = y_new;
    }

    // Draw a line connecting old with new
    // If position is 0 or value did not change, draw only a dot.
    if (_pos && (*y_old_ptr != y_new)) {
      display.drawLine(x+x_new, y+*y_old_ptr, x+x_new, y+y_new, color);
    } else {
      display.drawPixel(x+x_new, y+y_new, color);
    }

    // Update old position
    *y_old_ptr = y_new;
  }

  // Next x-position
  _pos++;
}

void PHN_LineGraph::update() {
}

void PHN_LineGraph::draw() {
  // Draw the frame
  display.drawRect(x, y, width, height, color(FRAME));

  // Fill the background by drawing from left to right
  // Do this starting at the current position to the end
  color_t bg = color(BACKGROUND);
  for (int dx = _pos; dx < (width - 2); dx++) {
    display.drawVerticalLine(x + dx + 1, y + 1, height - 2, bg);
  }
}
