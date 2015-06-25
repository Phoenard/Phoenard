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

#include "PHNButton.h"

void PHN_Button::setTextRaw(const char* text, int textLen) {
  setImage(TEXT_Image(text));
}

void PHN_Button::setImage(int index, const PHN_Image &image) {
  images[index] = image;
  invalidate();
}

void PHN_Button::setImagePalette(int index, const PHN_Palette &palette) {
  images[index].setPalette(palette);
  invalidate();
}

void PHN_Button::setImage(const PHN_Image &image) {
  for (unsigned char i = 0; i < 3; i++) {
    images[i] = image;
  }
  invalidate();
}

void PHN_Button::draw() {
  // Force-apply the default palette values
  const color_t colors_0[] = {color(FOREGROUND), color(FRAME), color(CONTENT)};
  const color_t colors_1[] = {color(HIGHLIGHT), color(FRAME), color(CONTENT)};
  const color_t colors_2[] = {color(ACTIVATED), color(FRAME), color(CONTENT)};

  setImagePalette(0, PHN_Palette(colors_0, 3));
  setImagePalette(1, PHN_Palette(colors_1, 3));
  setImagePalette(2, PHN_Palette(colors_2, 3));

  // Redraw the right state
  if (isClicked()) {
    images[2].draw(x, y, width, height);
  } else if (isTouched()) {
    images[1].draw(x, y, width, height);
  } else {
    images[0].draw(x, y, width, height);
  }
}

void PHN_Button::update() {
  if (isTouchChange()) {
    invalidate();
  }
}