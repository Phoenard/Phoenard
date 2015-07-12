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
 * @brief Contains the PHN_LineGraph widget; draws multiple line graphs
 */

#include "PHNWidget.h"

#ifndef _PHN_WIDGET_LINE_GRAPH_H_
#define _PHN_WIDGET_LINE_GRAPH_H_

/**
 * @brief Draws one or more line graphs
 *
 * After settings bounds, use setLineCount(int) to set how many lines are used.
 * You can then fill in the data using addValues(float*) by passing an array of
 * values for each line in the graph. When the end is reached, the graph is
 * cleared automatically for a continuous graph.
 */
class PHN_LineGraph : public PHN_Widget {
 public:
  /// Initializes a new line graph widget
  PHN_LineGraph() : _pos(0), autoClearDisabled(false) {}
  /// Sets how many lines are displayed
  void setLineCount(int nrLines);
  /// Sets the color of a line
  const void setLineColor(unsigned char lineIdx, color_t color) { _lineColors.set(lineIdx, color); }
  /// Sets the range of the values displayed
  void setRange(float minimum, float maximum);
  /// Gets the color set for a line
  const color_t lineColor(unsigned char lineIdx) { return _lineColors.get(lineIdx); }
  /// Gets the current position in the graph being drawn
  const unsigned int position(void) { return _pos; }
  /// Adds a new column of values; values being a value for each line
  void addValues(const float* values);
  /// Adds a new column of a line of values
  void addValue(float value);
  /// Sets whether the graph is automatically cleared every turn
  void setAutoClear(bool autoClear) { autoClearDisabled = !autoClear; }
  /// Clears the graph
  void clear(void);
  virtual void update(void);
  virtual void draw(void);
 private:
  unsigned char _dim;
  float _min, _max;
  int _pos;
  bool autoClearDisabled;
  DataBuffer _lastValues;
  PHN_Palette _lineColors;
};

#endif
