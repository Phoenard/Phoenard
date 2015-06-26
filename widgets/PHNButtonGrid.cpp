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

#include "PHNButtonGrid.h"

PHN_ButtonGrid::PHN_ButtonGrid() {
  this->cols = 0;
  this->rows = 0;
  this->count = 0;
  this->deleteAddedWidgets = true;
}

void PHN_ButtonGrid::setDimension(int columns, int rows) {
  this->rows = rows;
  this->cols = columns;
  setupCells();
}

void PHN_ButtonGrid::setSpacing(int spacing) {
  this->spacW = spacing;
  this->spacH = spacing;
  setupCells();
}

void PHN_ButtonGrid::setSpacing(int spacingW, int spacingH) {
  this->spacW = spacingW;
  this->spacH = spacingH;
  setupCells();
}

void PHN_ButtonGrid::setupCells() {
  // Resize the array of button widgets as needed
  int newCount = cols * rows;
  while (count < newCount) {
    count++;
    addWidget(*(new PHN_Button()));
  }
  setWidgetCapacity(newCount);

  // Calculate the widget width and height using the bounds and spacing
  cellW = (width  - (spacW * (cols - 1))) / cols;
  cellH = (height - (spacH * (rows - 1))) / rows;

  // Setup the bounds for each widget using the spacing, width and height
  int row, col;
  int c_x;
  int c_y = y;
  for (row = 0; row < rows; row++) {
    c_x = x;
    for (col = 0; col < cols; col++) {
      button(col, row).setBounds(c_x, c_y, cellW, cellH);
      c_x += cellW + spacW;
    }
    c_y += cellH + spacH;
  }
}

int PHN_ButtonGrid::getTouchedColumn() {
  return -1; //TODO
}

int PHN_ButtonGrid::getTouchedRow() {
  return -1; //TODO
}

int PHN_ButtonGrid::getTouchedIndex() {
  for (int i = 0; i < count; i++) {
     if (button(i).isTouched()) return i;
  }
  return -1;
}

int PHN_ButtonGrid::getClickedColumn() {
  return -1; //TODO
}

int PHN_ButtonGrid::getClickedRow() {
  return -1; //TODO
}

int PHN_ButtonGrid::getClickedIndex() {
  for (int i = 0; i < count; i++) {
     if (button(i).isClicked()) return i;
  }
  return -1;
}

int PHN_ButtonGrid::getIndex(int col, int row) {
  if (row == -1 || col == -1) return -1;
  return col + row * cols;
}

void PHN_ButtonGrid::update() {
  // No updates needed: all child button widgets update themselves
}

void PHN_ButtonGrid::draw() {
  // No draw needed: all child button widgets draw themselves
}