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

/**@file
 * @brief Contains the PHN_Image class for storing
 *        image information and several macros for creating images
 */

#include "PHNDisplay.h"
#include "PHNPalette.h"

#ifndef _PHN_IMAGE_H_
#define _PHN_IMAGE_H_

/**@brief Stores the information needed to draw an image to screen
 *
 * It stores a palette of colors to use when drawing, a function to call
 * to draw the function and user-defined data to pass along, such as a
 * path or pointer to the data being drawn.
 *
 * It is strongly discouraged
 * to pass the actual data being drawn with it, as a copy of the data is
 * stored on the heap, which may get copied over onto new instances
 * frequently. Keeping the data size minimal is recommended.
 */
class PHN_Image {
public:
  /// Null constructor used for copying one image to another
  PHN_Image() {}
  /// Constructs a new image with the draw function and uint32 data
  PHN_Image(void (*drawFunc)(int, int, int, int, PHN_Image&), uint32_t data);
  /// Constructs a new image with the draw function and text NULL-terminated data
  PHN_Image(void (*drawFunc)(int, int, int, int, PHN_Image&), const char* text);
  /// Constructs a new image with the draw function and data set
  PHN_Image(void (*drawFunc)(int, int, int, int, PHN_Image&), const void* data, int dataSize);
  /// Constructs a new image, using the data of the image specified
  PHN_Image(const PHN_Image &value);

  /// Gets the palette of colors set for the image
  const PHN_Palette &palette() { return _palette; }
  /// Gets a color in the palette of colors set for the image
  const color_t color(uint8_t index) { return _palette.get(index); }
  /// Sets a new palette of colors set for the image
  const void setPalette(const PHN_Palette &palette) { _palette = palette; }
  /// Gets the data associated with the image
  const void* data() const { return _data.data; }
  /// Gets the data associated with the image as text
  const char* text() const { return (const char*) _data.data; }
  /// Gets the data associated with the image as an integer
  const uint32_t data_int() const { return *((uint32_t*) _data.data); }
  /// Gets the data associated with the image as a memory pointer
  const void* data_ptr() const { return (void*) data_int(); }
  /// Gets the byte size of the data associated with the image
  const int dataSize() { return _data.dataSize; }

  /// Sets new data associated with the image
  void setData(const void* data, int dataSize) { _data.set(data, dataSize); }
  /// Sets new data associated with the image using a null-terminated String 
  void setData(const char* text) { _data.set((const void*) text, strlen(text)); }
  /// Calls the draw function to draw in the rectangle specified
  void draw(int x, int y, int width, int height) { _drawFunc(x, y, width, height, *this); }

private:
  PHN_Palette _palette;
  DataCopyBuffer _data;
  void (*_drawFunc)(int, int, int, int, PHN_Image&);
};

// Draw functions (not shown in doxygen)
//!@cond
void text_image_draw_func(int x, int y, int width, int height, PHN_Image &img);
void flash_image_draw_func(int x, int y, int width, int height, PHN_Image &img);
void flash_indexed_image_draw_func(int x, int y, int width, int height, PHN_Image &img);
//!@endcond

/// Macro for creating an image drawing text with a color border
#define TEXT_Image(text)  PHN_Image(text_image_draw_func, text)
/// Macro for creating an image drawing image data stored in flash
#define FLASH_Image(data)  PHN_Image(flash_image_draw_func, (uint32_t) (data))
/// Macro for creating an image drawing image data stored in flash, utilizing a color map
#define FLASH_MAPPED_Image(data)  PHN_Image(flash_indexed_image_draw_func, (uint32_t) (data))

#endif