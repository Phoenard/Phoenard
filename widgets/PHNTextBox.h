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
 * @brief Contains the PHN_TextBox widget; a text field the user can select and edit text in
 */

#include "PHNWidget.h"
#include "PHNScrollbar.h"

#ifndef _PHN_WIDGET_TEXT_H_
#define _PHN_WIDGET_TEXT_H_

#define PHN_WIDGET_TEXT_SCROLLWIDTH 16
#define PHN_WIDGET_TEXT_BLINKINTERVAL 600
#define PHN_WIDGET_TEXT_DRAGSELDELAY 1000

/**
 * @brief Shows an area of text that allows selections and dynamic editing
 *
 * Call setScrollbarVisible(bool), setTextSize(int) before use.
 * This makes sure all the styling is set up correctly. You can then proceed to set the
 * text and display it. You can make use of setSelection(const char*) to insert or replace the selections
 * the user has set in the field.
 *
 * The amount of columns and rows displayed is automatically calculated from the bounds.
 * To make it easier to specify the bounds to fit the text, you can make use of setDimension(int, int).
 * It automatically calculates the width and height to fit the text. Make sure you have set the text size
 * earlier, otherwise the results are unexpected.
 */
class PHN_TextBox : public PHN_Widget, public PHN_TextContainer {
 public:
  /// Initializes some defaults
  PHN_TextBox(void);
  /// Updates the width and height of the textbox to fit the columns and rows specified
  void setDimension(int columns, int rows);
  /// Sets the font size of the text
  void setTextSize(int size);
  /// Sets the maximum length of the text that can be displayed
  void setMaxLength(int length);
  /// Gets the font size of the text
  int textSize(void) { return this->_textSize; }
  /// Sets whether the scrollbar is displayed
  void setScrollbarVisible(bool visible);
  /// Gets the scrollbar widget used to scroll text
  PHN_Scrollbar &scrollbar(void) { return scroll; }
  /// Ensures the character specified is displayed
  bool ensureVisible(int charPosition);
  /// Gets the start of the user selection
  int selectionStart(void) { return selStart; }
  /// Gets the length of the user selection
  int selectionLength(void) { return selLength; }
  /// Sets up a new selection range for the user
  void setSelectionRange(int position, int length);
  /// Replaces the selected text with the text specified
  void setSelection(const char* text);
  /// Replaces the selected text with the character specified
  void setSelection(char character);
  /// Performs a backspace, deleting the selection or character before the cursor
  void backspace(void);
  
  using PHN_Widget::invalidate;
  
  /// Invalidates the text starting from a position
  void invalidate(int startPosition);
  /// Invalidates the text between two positions
  void invalidate(int startPosition, int endPosition);

  virtual void setTextRaw(const char* text, int textLen);
  virtual int textLength(void) { return this->length; }
  virtual const char* text(void) { return (char*) textBuff.data; }
  virtual void update(void);
  virtual void draw(void);
 private:
  void drawTextFromTo(int charStart, int charEnd, bool drawBackground);
  void drawCursor(bool visible);
  void updateScrollLimit(void);
  PHN_Scrollbar scroll;
  bool scrollVisible;
  int scrollOffset;
  int rows, cols;
  int chr_w, chr_h;
  int _textSize;
  int textAreaWidth;
  int length;
  int selLength, selStart, selEnd;
  int dragStart;
  long dragLastClick;
  int invalidateStart, invalidateEnd;
  bool invalidateAppended;
  int cursor_x, cursor_y;
  bool cursor_blinkVisible;
  long cursor_blinkLast;
  DataCopyBuffer textBuff;
};

#endif