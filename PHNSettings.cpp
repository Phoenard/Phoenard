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

#include "PHNSettings.h"

void PHN_Settings_ReadCali(PHN_Settings settings, int *hor_a, int *hor_b, int *ver_a, int *ver_b) {
  *hor_a = settings.touch_hor_a;
  *hor_b = 1023 - settings.touch_hor_b;
  *ver_a = settings.touch_ver_a;
  *ver_b = 1023 - settings.touch_ver_b;
  if (settings.flags & SETTINGS_TOUCH_HOR_INV) {
    *hor_a = 1023 - *hor_a;
    *hor_b = 1023 - *hor_b;
  }
  if (settings.flags & SETTINGS_TOUCH_VER_INV) {
    *ver_a = 1023 - *ver_a;
    *ver_b = 1023 - *ver_b;
  }
}

void PHN_Settings_WriteCali(PHN_Settings *settings, int hor_a, int hor_b, int ver_a, int ver_b) {
  if (hor_a > hor_b) {
    settings->flags |= SETTINGS_TOUCH_HOR_INV;
    settings->touch_hor_a = 1023 - hor_a;
    settings->touch_hor_b = hor_b;
  } else {
    settings->flags &= ~SETTINGS_TOUCH_HOR_INV;
    settings->touch_hor_a = hor_a;
    settings->touch_hor_b = 1023 - hor_b;
  }
  if (ver_a > ver_b) {
    settings->flags |= SETTINGS_TOUCH_VER_INV;
    settings->touch_ver_a = 1023 - ver_a;
    settings->touch_ver_b = ver_b;
  } else {
    settings->flags &= ~SETTINGS_TOUCH_VER_INV;
    settings->touch_ver_a = ver_a;
    settings->touch_ver_b = 1023 - ver_b;
  }
}

void PHN_loadSketch(const char* sketchName, bool loadNow) {
  // Write sketch load instruction to EEPROM
  PHN_Settings settings;
  PHN_Settings_Load(settings);
  for (unsigned char i = 0; i < 8; i++) {
    // Read the next character; fix NULL and lower case characters
    char c = (*sketchName) ? *(sketchName++) : ' ';
    if (c >= 'a' && c <= 'z') c -= ('a' - 'A');
    settings.sketch_toload[i] = c;
  }
  settings.flags |= SETTINGS_LOAD;
  PHN_Settings_Save(settings);
  
  // If set, reset the device using the watchdog timer
  if (loadNow) {
    WDTCSR = (1<<WDE) | (1<<WDCE);
    WDTCSR = (1<<WDE);
    for(;;);
  }
}