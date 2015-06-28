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
 * @brief Contains the PHN_ItemList widget; a widget that can show a list of any number of items
 */

#include "PHNWidget.h"

#ifndef _PHN_ITEM_LIST_H_
#define _PHN_ITEM_LIST_H_

/// Defines the delay between automatically scrolling up/down when touching
#define ITEMLIST_AUTO_SCROLL_DELAY 200

/**
 * @brief The item list widget shows a list of items
 *
 * This widget does not draw the items itself, rather, it allows the user
 * to pass in a function that performs the drawing of each item by index.
 * The only drawing this widget performs is the frame and the scrollbar.
 *
 * A single item can be selected by user interaction.
 *
 * If no function is specified, the index of the item is printed instead.
 *
 * To set a function, make a new global function and pass it to the widget
 * such as this:
 * itemList.setDrawFunction(drawMyItem);
 *
 * void drawMyItem(int x, int y, int w, int h, int index, boolean selected) {
 *   
 * }
 * 
 * Before use, call setItemCount() to specify the amount of items. Then use
 * setPageSize() to set how many items are displayed per page. Finally, set
 * the draw function to use for the items using setDrawFunction().
 */
class PHN_ItemList : public PHN_Widget {
 public:
  /// Initializes a new list
  PHN_ItemList();
  /// Sets the amount of items displayed
  void setItemCount(int itemCount);
  /// Gets the amount of items displayed
  int itemCount() const { return _itemCount; }
  /// Sets the amount of items displayed per page
  void setPageSize(int itemsPerPage);
  /// Gets the amount of items displayed per page
  int pageSize() const { return _pageSize; }
  /// Sets the draw function to use for the items
  void setDrawFunction(void (*drawFunction)(int, int, int, int, int, boolean));
  /// Sets the index of the selected item
  void setSelectedIndex(int selectedIndex);
  /// Gets the index of the selected item
  int selectedIndex() const { return _selectedIndex; }
  /// Checks whether the selected index was changed
  bool selectionChanged() const { return _selectedChanged; }
  /// Gets the index of the first item displayed
  int firstIndex() { return scroll.value(); }

  /// Forces a particular item to be re-drawn
  void drawItem(int index);

  virtual void update(void);
  virtual void draw(void);

 private:
  PHN_Scrollbar scroll;
  void (*_drawFunc)(int, int, int, int, int, boolean);
  int _itemCount, _pageSize;
  int _selectedIndex, _drawnSelIndex;
  int _itemW, _itemH;
  bool _selectedChanged;
  unsigned long lastScrollTime;
};

#endif