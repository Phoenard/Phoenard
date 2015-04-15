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

void PHN_Gauge::setRange(float minimum, float maximum) {
  _minimum = minimum;
  _maximum = maximum;
  setValue(_valueReq);
  invalidate();
}

void PHN_Gauge::setValue(float value) {
  if (value < _minimum) {
    _valueReq = _minimum;
  } else if (value > _maximum) {
    _valueReq = _maximum;
  } else {
    _valueReq = value;
  }
}
  
void PHN_Gauge::update() {
  if (_value == _valueReq) {
    return;
  }
  // Draw changes in the needle pointer (avoids having to draw the entire background)
  drawPointer(color(FOREGROUND));
  _value = _valueReq;
  drawPointer(color(CONTENT));
}

void PHN_Gauge::draw() {
  // Draw the gauge background circle
  int radius = width * 0.5;
  display.fillCircle(x + width / 2, y + width / 2, radius, color(FRAME));
  display.fillCircle(x + width / 2, y + width / 2, radius - 1, color(FOREGROUND));
  display.drawCircle(x + width / 2, y + width / 2, radius - 1, color(CONTENT));

  // Draw the pointer and background of value label field
  _value = _valueReq;
  drawPointer(color(CONTENT), true);

  // Draw the tick lines
  int tickCount = 11;
  for (int i = 0; i < tickCount; i++) {
    drawTickLine(radius - 2, radius - 8, (float) i /  (float) (tickCount - 1), color(CONTENT));
  }
}

void PHN_Gauge::drawPointer(color_t color, bool fillBg) {
  // Draw the line pointer
  float ang = (float) (_value - _minimum) / (float) (_maximum - _minimum);
  drawTickLine(0, 0.5 * width - 9, ang, color);
   
  // Draw the text
  int txt_h = 0.162 * width + (height - width);
  int txt_w = 0.73 * width + 2;
  int txt_x = x + (width - txt_w) / 2 + 1;
  int txt_y = y + height - txt_h + 1;
  if (fillBg) {
    display.fillRect(txt_x, txt_y, txt_w, txt_h, this->color(FOREGROUND));
    display.drawRect(txt_x, txt_y, txt_w, txt_h, this->color(FRAME));
    display.drawRect(txt_x + 1, txt_y, txt_w - 2, txt_h - 1, this->color(CONTENT));
  }
  char text[8];
  dtostrf(_value, 4, 2, text);
  display.setTextColor(color);
  display.drawStringMiddle(txt_x + 2, txt_y + 1, txt_w - 4, txt_h - 2, text);
}

void PHN_Gauge::drawTickLine(int r1, int r2, float ang, color_t color) {
  // Calculate angle, and from this the point to connect to
  float angle = PI * (1.5 * ang - 1.25);
  int c_x = x + 0.5 * width;
  int c_y = y + 0.5 * width;
  display.drawLine(c_x + r1 * cos(angle), c_y + r1 * sin(angle), c_x + r2 * cos(angle), c_y + r2 * sin(angle), color);
}