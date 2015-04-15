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
 * @brief Contains the PHN_ButtonGrid widget; multiple rows and columns of buttons
 */

#include "PHNWidget.h"
#include "PHNButton.h"

#ifndef _PHN_WIDGET_BUTTON_GRID_H_
#define _PHN_WIDGET_BUTTON_GRID_H_

/**
 * @brief Holds multiple rows and columns of buttons
 *
 * Before use, set the dimensions using setDimension(int, int) and set up the spacing
 * between buttons using setSpacing(int) or setSpacing(int, int). It is then possible
 * to access each button in the grid individually, set their state images and handle
 * their clicks.
 *
 * For ease of use, you can make use of getClickedIndex() to obtain an index of the
 * button currently clicked by the user. This index follows a Z-pattern, meaning that
 * the index increments per column and then per row. An index of -1 indicates no press.
 *
 * By default all buttons share the color palette set for this grid. Changing colors in
 * the grid will change it for all buttons. Once you set a color in an individual button,
 * this inheritance is lost.
 */
class PHN_ButtonGrid : public PHN_Widget {
 public:
  /// Basic constructor to set the initial rows/columns to 0
  PHN_ButtonGrid(void);
  /// Sets the amount of rows and columns in the grid
  void setDimension(int rows, int columns);
  /// Sets the horizontal and vertical spacing between buttons
  void setSpacing(int spacing);
  /// Sets the horizontal and vertical spacing between buttons
  void setSpacing(int spacingW, int spacingH);

  /// Gets the amount of buttons stored
  int buttonCount() const { return count; }

  /// Accesses a single button in the grid by index
  PHN_Button &button(int index) { return *((PHN_Button*) widget(index)); }
  /// Accesses a single button in the grid by column and row
  PHN_Button &button(int row, int column) { return button(getIndex(row, column)); }

  /// Gets the column of the cell pressed, -1 if none is pressed
  int getTouchedColumn(void);
  /// Gets the row of the cell pressed, -1 if none is pressed
  int getTouchedRow(void);
  /// Gets the index of the cell pressed, -1 if none is pressed
  int getTouchedIndex(void);

  /// Gets the column of the cell clicked, -1 if none is clicked
  int getClickedColumn(void);
  /// Gets the row of the cell clicked, -1 if none is clicked
  int getClickedRow(void);
  /// Gets the index of the cell clicked, -1 if none is clicked
  int getClickedIndex(void);

  virtual void update(void);
  virtual void draw(void);
 protected:
  int cols, rows, count, cellW, cellH, spacW, spacH;
  int getIndex(int row, int col);
 private:
  void setupCells();
};

#endif