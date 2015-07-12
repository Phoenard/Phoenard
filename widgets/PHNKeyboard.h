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
 * @brief Contains the PHN_Keyboard widget; a grid of keys
 */

#include "PHNWidget.h"

#ifndef _PHN_KEYBOARD_H_
#define _PHN_KEYBOARD_H_

/**
 * @brief The keyboard widget shows character-labeled keys
 *
 * This widget can show a keyboard layout with a single character
 * clickable on every key. The user can press and release on a key
 * to initiate a key-press. This functionality can be useful to add
 * a field where the user can enter text or numeric input.
 *
 * This keyboard supports multiple formats. You can add multiple
 * key layout using addKeys() also specifying the name of the format.
 * By adding '\\f' keys to the layout, the user can switch layouts using
 * the keyboard. In place of these keys, the next format to switch to
 * is displayed. For simple operation it is sufficient to use setKeys()
 * with the character mapping to use, ignoring the multi-format support.
 *
 * Setup the dimension, spacing and keys of this widget before use.
 * For key buttons spanning multiple keys, use the same character in
 * a row in the key mapping.
 */
class PHN_Keyboard : public PHN_Widget {
 public:
  /// Initializes a new Keyboard
  PHN_Keyboard();
  /// Sets the amount of columns and rows in the grid
  void setDimension(int columns, int rows);
  /// Sets the horizontal and vertical spacing between buttons
  void setSpacing(int spacing);
  /// Sets the horizontal and vertical spacing between buttons
  void setSpacing(int spacingW, int spacingH);
  /// Sets all the character keys to display
  void setKeys(const char* keyChars);
  /// Adds a new keyboard format selectable using the \f key
  void addKeys(const char* keyChars);
  /// Adds a new keyboard format selectable using the \f key
  void addKeys(const char* formatName, const char* keyChars);
  /// Clears all key formats added
  void clearKeys();
  /// Gets the currently used format index
  int formatIndex() const { return this->formatIdx; }
  /// Sets the currently used format index
  void setFormatIndex(int index);
  /// Switches the keyboard to the next key format
  void nextFormat();
  /// Obtains the character for a given key index
  char key(int index);
  /// Obtains the key character index clicked, -1 when there is none
  int clickedIndex() const { return this->clickedIdx; }
  /// Obtains the key character clicked, NULL when there is none
  char clickedKey();

  virtual void undraw(void);
  virtual void update(void);
  virtual void draw(void);
 private:
  void setupCells();
  char fmt_key(int fmtIndex, int index);
  void updateCell(int idx, bool drawCell, bool eraseCell);
  DataBuffer formatChars;
  DataBuffer formatNames;
  unsigned char cols, rows;
  unsigned char cellW, cellH, spacW, spacH;
  int count;
  int formatCnt;
  int formatIdx;
  int pressedIdx;
  int clickedIdx;
};

#endif