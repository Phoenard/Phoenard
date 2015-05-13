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
  _baseVal = minimum;
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

void PHN_BarGraph::setBaseValue(float baseValue) {
  _baseVal = baseValue;
  invalidate();
}

void PHN_BarGraph::update() {
  // If invalidated; ignore drawing updates
  if (invalidated) return;

  // If value flips around the base, make sure to clear properly
  if ((_valueReq > _baseVal) != (_value > _baseVal)) {
    drawBar(_value, _baseVal, color(FOREGROUND));
    _value = _baseVal;
  }

  // Draw changes in the bar graph; invalidate() causes flickering
  if (_valueReq != _value) {
    // Whether bar is increased or reduced depends on two factors
    if ((_valueReq >= _baseVal) != (_valueReq > _value)) {
      drawBar(_value, _valueReq, color(FOREGROUND));
    } else {
      drawBar(_value, _valueReq, color(CONTENT));
    }
  }

  _value = _valueReq;
}

void PHN_BarGraph::draw() {
  // Fill the background with a rectangle and a frame
  display.drawRect(x, y, width, height, color(FRAME));
  display.fillRect(x+1, y+1, width-2, height-2, color(FOREGROUND));
  
  // Draw the bar contents itself
  _value = _valueReq;
  drawBar(_baseVal, _value, color(CONTENT));
}

void PHN_BarGraph::drawBar(float val_a, float val_b, color_t color) {
  // Ensure val_a < val_b by swapping as needed
  if (val_a > val_b) {
    float f = val_b;
    val_b = val_a;
    val_a = f;
  }
  // Fill the area between val_a and val_b with a bar
  // Start by converting the input parameters into 0-1 space
  val_a = (val_a - _minimum) / (_maximum - _minimum);
  val_b = (val_b - _minimum) / (_maximum - _minimum);
  // Calculate the bar area
  int bar_x1, bar_x2, bar_y1, bar_y2;
  // Horizontal or vertical mode?
  if (width > height) {
    int bar_w = width - 2;
    int bar_h = height - 4;
    bar_y1 = 2;
    bar_y2 = bar_h + bar_y1;
    bar_x1 = 1 + val_a * bar_w;
    bar_x2 = 1 + val_b * bar_w;
  } else {
    int bar_w = width - 4;
    int bar_h = height - 2;
    bar_x1 = 2;
    bar_x2 = bar_w + bar_x1;
    bar_y1 = 1 + bar_h - val_b * bar_h;
    bar_y2 = 1 + bar_h - val_a * bar_h;
  }
  display.fillRect(x + bar_x1, y + bar_y1, bar_x2 - bar_x1, bar_y2 - bar_y1, color);
}