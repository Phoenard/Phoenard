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

void PHN_BarGraph::setRange(float minimum, float maximum) {
  _minimum = minimum;
  _maximum = maximum;
  setValue(_valueReq);
  invalidate();
}

void PHN_BarGraph::setValue(float value) {
  if (value < _minimum) {
    _valueReq = _minimum;
  } else if (value > _maximum) {
    _valueReq = _maximum;
  } else {
    _valueReq = value;
  }
}
  
void PHN_BarGraph::update() {
  // Draw changes in the bar graph; invalidate() causes flickering
  if (_valueReq > _value) {
    // Value increased - bar increases
    drawBar(_value, _valueReq, color(CONTENT));
  } else if (_valueReq < _value) {
    // Value decreased - bar decreases
    drawBar(_valueReq, _value, color(FOREGROUND));
  }
  _value = _valueReq;
}

void PHN_BarGraph::draw() {
  // Fill the background with a rectangle and a frame
  display.drawRect(x, y, width, height, color(FRAME));
  display.fillRect(x+1, y+1, width-2, height-2, color(FOREGROUND));
  
  // Draw the bar contents itself
  _value = _valueReq;
  drawBar(_minimum, _value, color(CONTENT));
}

void PHN_BarGraph::drawBar(float val_a, float val_b, color_t color) {
  // Fill the area between val_a and val_b with a bar
  // Start by converting the input parameters into 0-1 space
  val_a = (val_a - _minimum) / (_maximum - _minimum);
  val_b = (val_b - _minimum) / (_maximum - _minimum);
  // Calculate the bar area
  int bar_w = width - 2;
  int bar_h = height - 2;
  int bar_x1, bar_x2, bar_y1, bar_y2;
  // Horizontal or vertical mode?
  if (bar_w > bar_h) {
    bar_y1 = 1;
    bar_y2 = bar_h + bar_y1;
    bar_x1 = 1 + val_a * bar_w;
    bar_x2 = 1 + val_b * bar_w;
  } else {
    bar_x1 = 1;
    bar_x2 = bar_w + bar_x1;
    bar_y1 = 1 + bar_h - val_b * bar_h;
    bar_y2 = 1 + bar_h - val_a * bar_h;
  }
  display.fillRect(x + bar_x1, y + bar_y1, bar_x2 - bar_x1, bar_y2 - bar_y1, color);
}