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
 * @brief Contains the PHN_Palette, a color container, and several palette macros
 */

#include "PHNDisplayHardware.h"
#include <utility/PHNUtils.h>

#ifndef _PHN_PALETTE_H_
#define _PHN_PALETTE_H_

/**@brief A color Palette that can store up to 256 colors
 *
 * Getting colors outside the range set returns BLACK.
 * Setting colors outside the current range resizing the
 * palette to fit the new indices.
 */
class PHN_Palette {
public:
  /// Constructs a new empty palette
  PHN_Palette() {}
  /// Constructs a new palette with the colors specified
  PHN_Palette(const color_t* colors, int colorCount);
  /// Sets all the colors for the palette at once
  void setAll(const color_t* colors, int colorCount);
  /// Sets a single color at an index in the palette
  void set(int index, color_t color);
  /// Gets a single color set at the index, BLACK if index out of range
  color_t get(int index) const;
  /// Gets how many colors are stored inside this palette
  int count() const { return _colorData.dataSize / sizeof(color_t); }
  /// Gets access to the raw color_t array data stored
  color_t* data() const { return (color_t*) _colorData.data; }

private:
  DataCopyBuffer _colorData;
 
};

// Macro to quickly turn a list of colors into a PHNPalette instance
/**@brief Macro to turn a list of colors in a PHN_Palette
 * 
 * Usage: PHN_Palette pal = PALETTE(RED, BLACK, BLUE);
 */
#define PALETTE(...)  PHN_Palette((const color_t[]) {__VA_ARGS__}, sizeof((const color_t[]) {__VA_ARGS__}) / sizeof(color_t))

// Default constants for use in widgets and whatnot
// PALETTE([BACKGROUND], [FRAME/FOREGROUND], [CONTENT/TEXT])
/**@name Palette constants
 * @brief Macros for default image state palettes
 */
//@{
#define PALETTE_NORMAL   PALETTE(WHITE,  RED, BLACK)
#define PALETTE_PRESSED  PALETTE(YELLOW, RED, BLACK)
#define PALETTE_CLICKED  PALETTE(BLUE,   RED, WHITE)
//@}

#endif