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

#include "PHNTextContainer.h"

void PHN_TextContainer::setText(String textString) {
  int len = textString.length();
  char *textArr = new char[len+1];
  textString.toCharArray(textArr, len+1);
  setTextRaw(textArr, len);
  delete[] textArr;
}

void PHN_TextContainer::setText(long valueText) {
  char buff[15];
  ltoa(valueText, buff, 10);
  setText(buff);
}

void PHN_TextContainer::setText(double valueText) {
  char buff[15];
  dtostrf(valueText, 5, 3, buff);
  setText(buff);
}

void PHN_TextContainer::setText(const char* text) {
  setTextRaw(text, (int) strlen(text));
}

void PHN_TextContainer::clearText() {
  setTextRaw("", 0);
}

int PHN_TextContainer::textLength() {
  return (int) strlen(text());
}