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

#include "PHNItemList.h"

static void itemlist_default_draw_func(int x, int y, int w, int h, int index, boolean selected) {
  color_t bgcolor = (selected ? YELLOW : WHITE);
  char text[7];
  itoa(index+1, text, 10);
  display.setTextColor(BLACK, bgcolor);
  display.fillRect(x, y, w, h, bgcolor);
  display.drawStringMiddle(x, y, w, h, text);
}

PHN_ItemList::PHN_ItemList() {
  _drawFunc = itemlist_default_draw_func;
  scroll.setRange(0, 0);
  addWidget(scroll);
}

void PHN_ItemList::setItemCount(int itemCount) {
  _itemCount = itemCount;
  invalidate();
}

void PHN_ItemList::setPageSize(int itemsPerPage) {
  _pageSize = itemsPerPage;
  invalidate();
}

void PHN_ItemList::setDrawFunction(void (*drawFunction)(int, int, int, int, int, boolean)) {
  _drawFunc = drawFunction;
  invalidate();
}

void PHN_ItemList::setSelectedIndex(int selectedIndex) {
  if (selectedIndex < 0) {
    selectedIndex = 0;
  } else if (selectedIndex >= _itemCount) {
    selectedIndex = _itemCount - 1;
  }
  if (_selectedIndex != selectedIndex) {
    _selectedIndex = selectedIndex;
    _selectedChanged = true;
  }
}
  
void PHN_ItemList::drawItem(int index) {
  int k = (index-scroll.value());
  if (k >= 0 && k < _pageSize) {
    bool selected = (index == _selectedIndex);
    int x = this->x+1;
    int y = this->y+1+k*(_itemH+1);
    if (index >= 0 && index < _itemCount) {
      _drawFunc(x, y, _itemW, _itemH, index, selected);
    } else {
      display.fillRect(x, y, _itemW, _itemH, color(BACKGROUND));
    }
  }
}

void PHN_ItemList::update() {
  if (invalidated) {
    _itemH = (height-2) / _pageSize;
    _itemW = (width-_itemH);
    scroll.setBounds(x+_itemW+1, y, _itemH+2, 1+_pageSize*(_itemH+1));
    scroll.setRange(max(0, _itemCount-_pageSize), 0);
  }
  _selectedChanged = false;

  // Use touch input to update the selected index
  bool autoScrollUp = false;
  bool autoScrollDown = false;
  if (display.isTouched(x+1, y+1, _itemW, _pageSize*(_itemH+1))) {
    int k = (display.getTouch().y-y-1) / (_itemH+1);
    autoScrollUp = (k == 0);
    autoScrollDown = (k==(_pageSize-1));
    setSelectedIndex(k + scroll.value());
  }
  if (!autoScrollUp && !autoScrollDown) {
    lastScrollTime = millis();
  } else if ((millis() - lastScrollTime) >= ITEMLIST_AUTO_SCROLL_DELAY) {
    if (autoScrollUp) {
      scroll.setValue(scroll.value() - 1);
      setSelectedIndex(scroll.value());
    } else if (autoScrollDown) {
      scroll.setValue(scroll.value() + 1);
      setSelectedIndex(scroll.value() + _pageSize - 1);
    }
    lastScrollTime = millis();
  }

  // Invalidate when scroll changes
  if (scroll.isValueChanged()) {
    invalidate();
  }

  // Check that the selected index is within the scrollable range
  int idx_first = scroll.value();
  int idx_last = idx_first+_pageSize-1;
  if (_selectedIndex < idx_first) {
    setSelectedIndex(idx_first);
  } else if (_selectedIndex > idx_last) {
    setSelectedIndex(idx_last);
  }

  // Perform partial redraws when selection changes here
  if (!invalidated && _drawnSelIndex != _selectedIndex) {
    drawItem(_drawnSelIndex);
    drawItem(_selectedIndex);
  }
  _drawnSelIndex = _selectedIndex;
}

void PHN_ItemList::draw() {
  // Draw the frame for the items
  color_t frameColor = color(FRAME);
  display.drawRect(x, y, _itemW+2, 1+_pageSize*(_itemH+1), frameColor);
  
  int lastY = y + 1;
  for (int i = 1; i < min(_pageSize, _itemCount+1); i++) {
    lastY += _itemH + 1;
    display.drawHorizontalLine(x+1, lastY-1, _itemW, frameColor);
  }
  if (_itemCount < _pageSize) {
    display.fillRect(x+1, lastY, _itemW, (_pageSize-_itemCount)*(_itemH+1)-1, color(BACKGROUND));
  }
  

  // Draw all visible items
  for (int i = scroll.value(); i < _itemCount; i++) {
    drawItem(i);
  }
}