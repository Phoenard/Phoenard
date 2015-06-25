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

/**
 * @file
 * @brief Contains the PHN_Button widget; a pressable three-state button
 */

#include "PHNWidget.h"
#include "PHNImage.h"

#ifndef _PHN_BUTTON_H_
#define _PHN_BUTTON_H_

/**
 * @brief The button widget can be clicked by the user, showing three states
 *
 * The button's text or three individual image states (normal, pressed, clicked) can be set.
 * After adding to a display, routinely checking the isClicked() function allows you to see
 * if the user clicked the button. Touch input is handled automatically as long as you update
 * the display routinely as well.
 */
class PHN_Button : public PHN_Widget, public PHN_TextContainer {
public:
  /// Gets a state image set, where index 0=normal, 1=pressed, 2=clicked
  const PHN_Image &image(int index) { return images[index]; }
  /// Sets a state image, where index 0=normal, 1=pressed, 2=clicked
  void setImage(int index, const PHN_Image &image);
  /// Sets an image for all three states, using default colors to draw with
  void setImage(const PHN_Image &image);
  /// Change for individual states what colors are used, where index 0=normal, 1=pressed, 2=clicked
  void setImagePalette(int index, const PHN_Palette &palette);

  virtual void setTextRaw(const char* text, int textLen);
  virtual const char* text() { return ""; }
  virtual void draw(void);
  virtual void update(void);

private:
  PHN_Image images[3];
};

#endif