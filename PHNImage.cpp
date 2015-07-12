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

#include "PHNImage.h"

PHN_Image::PHN_Image(void (*drawFunc)(int, int, int, int, PHN_Image&), uint32_t data) {
  this->_drawFunc = drawFunc;
  this->_palette = PALETTE(BLACK, RED, WHITE);
  this->setData((void*) &data, 4);
}

PHN_Image::PHN_Image(void (*drawFunc)(int, int, int, int, PHN_Image&), const char* text) {
  this->_drawFunc = drawFunc;
  this->_palette = PALETTE(BLACK, RED, WHITE);
  this->setData((const void*) text, strlen(text) + 1);
}

PHN_Image::PHN_Image(void (*drawFunc)(int, int, int, int, PHN_Image&), const void* data, int dataSize) {
  this->_drawFunc = drawFunc;
  this->_palette = PALETTE(BLACK, RED, WHITE);
  this->setData(data, dataSize);
}

PHN_Image::PHN_Image(const PHN_Image &value) {
  this->_palette = value._palette;
  this->_drawFunc = value._drawFunc;
  this->_data = value._data;
}

void text_image_draw_func(int x, int y, int width, int height, PHN_Image &img) {
  int rect_rad = min(width, height)/4;

  display.fillBorderRoundRect(x, y, width, height, rect_rad, img.color(0), img.color(1));
  display.setTextColor(img.color(2), img.color(0));
  display.drawStringMiddle(x, y, width, height, img.text());
}

void flash_image_draw_func(int x, int y, int width, int height, PHN_Image &img) {
  FlashMemoryStream stream(img.data_ptr(), 0xFFFFFFFF);
  display.drawImage(stream, x, y);
}

void flash_indexed_image_draw_func(int x, int y, int width, int height, PHN_Image &img) {
  FlashMemoryStream stream(img.data_ptr(), 0xFFFFFFFF);
  display.drawImage(stream, x, y, img.palette().data());
}