#include "PHNTextBox.h"

PHN_TextBox::PHN_TextBox() {
  this->length = 0;
  this->selStart = 0;
  this->selLength = 0;
  this->selEnd = 0;
  this->invalidateStart = -1;
  this->invalidateEnd = -1;
  this->cursor_x = -1;
  this->cursor_y = -1;
  this->cursor_blinkLast = 0;
  this->cursor_visible = true;
  this->scrollOffset = 0;
  this->dragStart = -1;
  this->setTextSize(2);
  this->setMaxLength(100);
  
  // Default scrollbar properties
  this->scroll.setRange(0, 0);
  this->scroll.setVisible(false);
  this->addWidget(this->scroll);
  
  // Default backspace button properties
  this->backspaceBtn.setText("\b");
  this->backspaceBtn.setVisible(false);
  this->addWidget(this->backspaceBtn);
}

void PHN_TextBox::setDimension(int columns, int rows) {
  // Use the known column/row/scrollbar states to calculate the bounds
  int width = columns*chr_w+2*_textSize+2;
  int height = rows*chr_h+2*_textSize+2;
  // Adjust width for a visible scrollbar
  if (scroll.isVisible()) {
    if (rows > 1) {
      // Multiple rows - vertical scrollbar
      width += chr_h+1;
    } else {
      // Only a single row with scrollbar on the right
      width += height*2+1;
    }
  }
  
  // Adjust width for a visible backspace button
  if (backspaceBtn.isVisible()) {
    width += _textSize*20+5;
  }
  setSize(width, height);
}

void PHN_TextBox::setMaxLength(int length) {
  if (length < this->length) {
     this->length = length;
  }
  textBuff.resize(length + 1);
}

void PHN_TextBox::setTextSize(int size) {
  _textSize = size;
  chr_w = _textSize * 6;
  chr_h = _textSize * 8;
  invalidate();
}

void PHN_TextBox::showScrollbar(bool visible) {
  scroll.setVisible(visible);
  invalidate();
}

void PHN_TextBox::showBackspace(bool visible) {
  backspaceBtn.setVisible(visible);
  invalidate();
}

void PHN_TextBox::setTextRaw(const char* text, int textLen) {
  length = min(textBuff.dataSize-1, textLen);
  memcpy(textBuff.data, text, sizeof(char) * length);
  ((char*) textBuff.data)[length] = 0;
  updateScrollLimit();
  setSelectionRange(length, 0);
  invalidate();
}

void PHN_TextBox::showCursor(bool visible) {
  cursor_visible = visible;
}

bool PHN_TextBox::ensureVisible(int charPosition) {
  int col = 0;
  int row = -scrollOffset;
  char* text = (char*) textBuff.data;
  for (int i = 0; i <= length; i++) {
    if (text[i] == '\r')
      continue;

    if (text[i] == '\n' || (col >= cols)) {
      row++;
      col = 0;
    }
    if (i == charPosition) {
      break;
    }

    if (text[i] != '\n')
      col++;
  }

  // Go one row back for single-row text fields to properly show last character
  if ((rows == 1) && (col == 0) && (text[charPosition-1] != '\n')) {
    row--;
  }

  // Adjust scroll position
  int newScroll = scrollOffset;
  if (row < 0) {
    // Scroll up the required amount of rows
    newScroll += row;
  } else if (row >= rows) {
    // Scroll down the required amount of rows
    newScroll += row - rows + 1;
  }
  scroll.setValue(newScroll);
  return newScroll != scrollOffset;
}

void PHN_TextBox::updateScrollLimit() {
  int row = 0;
  int col = 0;
  char* text = (char*) textBuff.data;
  for (int i = 0; i <= length; i++) {
    if (text[i] == '\r')
      continue;
    
    if (text[i] == '\n' || col >= cols) {
      row++;
      col = 0;
    }
    if (text[i] != '\n')
      col++;
  }
  updateScrollLimit(col, row);
}

void PHN_TextBox::updateScrollLimit(int col, int row) {
  // Update scroll maximum based on the amount of rows
  int scrollMax = row - rows + 1;

  // Ensure above 0
  if (scrollMax < 0) scrollMax = 0;

  // Handle inversion for single-row scrollbar
  if (rows == 1) {
    scroll.setRange(0, scrollMax);
  } else {
    scroll.setRange(scrollMax, 0);
  }
}

void PHN_TextBox::setSelectionRange(int position, int length) {
  // Protection against selection past the limit
  if (position >= this->length) {
    position = this->length;
    length = 0;
  }

  // If unchanged, do nothing
  if (position == selStart && length == selLength) {
    return;
  }
  int end = position + length;

  // Perform character invalidation (redrawing)
  if (!selLength && !length) {
    // When only the cursor changes, only redraw the cursor
    // This is done by re-drawing past the text length limit
    invalidate(this->length);
  } else if (length && !selLength) {
    // No previous selection to a selection
    invalidate(position, position+length);
  } else if (!length && selLength) {
    // Previous selection to no selection
    invalidate(selStart, selEnd);
  } else {
    // Undo edges in previous selection
    if (position > selStart)
      invalidate(selStart, position);
    if (end < selEnd)
      invalidate(end, selEnd);

    // Add changes of the new selection
    if (position > selEnd)
      invalidate(position, end);
    if (position < selStart)
      invalidate(position, selStart);
    if (end > selEnd)
      invalidate(selEnd, end);
  }
  
  // Update selection information
  selStart = position;
  selLength = length;
  selEnd = end;
}

void PHN_TextBox::setSelection(char character) {
  const char seltext[] = {character, 0};
  setSelection(seltext);
}

void PHN_TextBox::backspace() {
  if (selLength) {
    const char seltext[] = {0};
    setSelection(seltext);
  } else if (selStart) {
    // Shift ending one to the left
    char* text = (char*) textBuff.data;
    memmove(text+selStart-1, text+selStart, length-selStart);
    length -= 1;
    text[length] = 0;
    updateScrollLimit();
    setSelectionRange(selStart-1, 0);
    ensureVisible(selStart);
    invalidate(selStart);
  }
}

void PHN_TextBox::setSelection(const char* selectionText) {
  // Handle backspace characters here
  while (*selectionText == '\b') {
    backspace();
    selectionText++;
  }

  // Now enter the actual text
  int len = min((int) strlen(selectionText), (int) (textBuff.dataSize-length+selLength));
  char* text = (char*) textBuff.data;
  bool appended = (selLength == 0 && selStart == length);

  // If nothing is set, do nothing
  if (!len && (selLength == 0)) return;
  
  // Shift everything after the selection to the right
  memmove(text+selStart+len, text+selEnd, length-selEnd);

  // Insert the text value
  memcpy(text+selStart, selectionText, len);

  // Update length
  length = length - selLength + len;
  text[length] = 0;
  updateScrollLimit();

  // Invalidate the changed area
  if (len == selLength) {
    invalidate(selStart, selEnd);
  } else {
    invalidate(selStart);
  }

  // Update the start and length of selection
  setSelectionRange(selStart+len, 0);
  ensureVisible(selStart);

  // If text was appended (cursor at the end), use faster redraw
  invalidateAppended = appended;
}

void PHN_TextBox::invalidate(int startPosition) {
  invalidate(startPosition, length);
}

void PHN_TextBox::invalidate(int startPosition, int endPosition) {
  invalidateAppended = false;
  if (invalidateStart == -1 || invalidateStart > startPosition)
    invalidateStart = startPosition;
  if (invalidateEnd == -1 || invalidateEnd < endPosition)
    invalidateEnd = endPosition;
}

void PHN_TextBox::update() {
  // Update scrollbar layout changes
  if (invalidated) {
    // Update row count
    rows = (height-2*_textSize-2) / chr_h;

    // Update width and column count, applying this to the UI
    int scrollWidth = 0;
    int backspaceWidth = 0;
    int backspaceHeight = (rows==1) ? height : (chr_h+2*_textSize+2);
    if (scroll.isVisible()) {
      if (rows > 1) {
        scrollWidth = chr_h+3;
      } else {
        scrollWidth = height*2+3;
      }
    }
    if (backspaceBtn.isVisible()) {
      backspaceWidth = _textSize*20+5;
    }
    textAreaWidth = (width - scrollWidth - backspaceWidth);
    cols = (textAreaWidth-2*_textSize-2) / chr_w;
    scroll.setBounds(x+textAreaWidth-1, y, scrollWidth, height);
    backspaceBtn.setBounds(x+width-backspaceWidth+4, y, backspaceWidth-5, backspaceHeight);
  }

  // Handle backspace clicks
  if (backspaceBtn.isClicked()) backspace();

  // Handle Touch selection changes
  char* text = (char*) textBuff.data;
  if (cursor_visible && display.isTouched(x+_textSize+1, y+_textSize+1, cols*chr_w, rows*chr_h)) {
    PressPoint pos = display.getTouch();
    int posRow = (pos.y-(this->y+_textSize+1)) / chr_h;

    // Go by all characters until found
    int x;
    int col = 0;
    int row = -scrollOffset;
    int pressedIdx = this->length;
    for (int i = 0; i <= length; i++) {
      if (text[i] == '\r')
        continue;

      if (col >= cols) {
        row++;
        col = 0;
      }
      if (row == posRow) {
        x = this->x + _textSize + 1 + col * chr_w;
        if ((text[i] == '\n') || (pos.x <= x+(chr_w>>1))) {
          pressedIdx = i;
          break;
        } else if (col == (cols-1)) {
          pressedIdx = i+1;
          break;
        }
      }
      if (text[i] == '\n') {
        row++;
        col = 0;
      } else {
        col++;
      }
    }
    if (display.isTouchDown()) {
      // Drag start
      dragStart = pressedIdx;
      setSelectionRange(pressedIdx, 0);
      dragLastClick = millis();
    } else if (dragStart != -1 && (millis() - dragLastClick) > PHN_WIDGET_TEXT_DRAGSELDELAY) {
      // Drag selection
      int start = min(dragStart, pressedIdx);
      int end = max(dragStart, pressedIdx);
      setSelectionRange(start, max(1, end-start));
    } else if (dragStart != pressedIdx) {
      // Repositioned the character
      dragStart = pressedIdx;
      setSelectionRange(pressedIdx, 0);
      dragLastClick = millis();
    }
    ensureVisible(pressedIdx);
  } else if (!display.isTouched()) {
    dragStart = -1;
  }

  // Update scrolling
  if (scrollOffset != scroll.value()) {
    scrollOffset = scroll.value();
    invalidate();
  }

  // Partial redraws
  if (!invalidated) {
    // Redraw parts of changed text
    if (invalidateStart != -1) {
      drawTextFromTo(invalidateStart, invalidateEnd, !invalidateAppended);
    }

    // Handle cursor blinking
    bool updateCursor = false;
    if ((millis() - cursor_blinkLast) >= PHN_WIDGET_TEXT_BLINKINTERVAL) {
      updateCursor = true;
    }
    if (!cursor_visible) {
      updateCursor = cursor_blinkVisible;
    }
    if (updateCursor) {
      cursor_blinkLast = millis();
      drawCursor(!cursor_blinkVisible);
    }
  }
  invalidateStart = -1;
  invalidateEnd = -1;
  invalidateAppended = false;
}

void PHN_TextBox::draw() {
  // Draw background color and grid
  display.fillBorderRect(x, y, textAreaWidth, height, color(FOREGROUND), color(FRAME));

  // Draw text
  drawTextFromTo(0, this->length, false);
}

void PHN_TextBox::drawCursor(bool visible) {
  cursor_blinkVisible = visible;
  if (cursor_x != -1 && cursor_y != -1) {
    color_t clr = cursor_blinkVisible ? color(CONTENT) : color(FOREGROUND);
    display.fillRect(cursor_x, cursor_y, _textSize, _textSize*8, clr);
  }
}

void PHN_TextBox::drawTextFromTo(int charStart, int charEnd, bool drawBackground) {
  // Reset cursor blinking to draw next update
  cursor_blinkLast = millis() - PHN_WIDGET_TEXT_BLINKINTERVAL;

  // When all empty, just wipe the screen and reset
  if (!length) {
    scroll.setRange(0, 0);
    display.fillRect(x+1, y+1, textAreaWidth-2, height-2, color(FOREGROUND));
    cursor_x = x+_textSize+1;
    cursor_y = y+_textSize+1;
    selStart = 0;
    selLength = 0;
    return;
  }

  // First hide cursor to prevent glitches
  if (cursor_blinkVisible) {
    drawCursor(false);
  }

  Viewport old = display.getViewport();
  display.setViewport(x+_textSize+1, y+_textSize+1, width, height);

  // Draw selection highlight, cursor and text
  int row = -scrollOffset;
  int col = 0;
  int x, y;
  bool charSel;
  cursor_x = -1;
  cursor_y = -1;
  char* text = (char*) textBuff.data;
  for (int i = 0; i <= length; i++) {
    if (text[i] == '\r')
      continue;

    if (col >= cols) {
      row++;
      col = 0;
    }
    if (row >= 0 && row < rows) {
      x = col * chr_w;
      y = row * chr_h;
      
      if (i == selStart && !selLength) {
        // Set up cursor
        cursor_x = display.getViewport().x + x + 1 - _textSize - (_textSize>>1);
        
        cursor_y = display.getViewport().y + y;
      }
      
      // Only do drawing operations in the selected range
      if (i >= charStart && i <= charEnd) {
        charSel = i >= selStart && i < (selStart+selLength);

        // Fill the current row and all rows below with background color
        if (drawBackground && charEnd >= length) {
          // If last character of current line, clear right of character
          if ((i == length) || (text[i] == '\n')) {
            display.fillRect(x-1, y, (cols-col)*chr_w+1, chr_h, color(FOREGROUND));
          }

          // If last character of all text, wipe the remaining rows
          if (i == length) {
            display.fillRect(-1, y+chr_h, cols*chr_w+1, chr_h*(rows-row-1), color(FOREGROUND));
          }
        }

        // Only do drawing when not a newline
        if (text[i] != '\n' && text[i]) {
          // Update text color based on selection highlight
          color_t bgColor = color(charSel ? HIGHLIGHT : FOREGROUND);
          display.setTextColor(color(CONTENT), bgColor);

          // Draw text with a border for highlight updates
          int border_s = _textSize>>1;
          display.drawChar(x, y, text[i], _textSize);
          display.fillRect(x-border_s, y, border_s, chr_h, bgColor);
          display.fillRect(x+chr_w-_textSize, y, border_s, chr_h, bgColor);
        }
      }
    }
    
    if (text[i] == '\n') {
      row++;
      col = 0;
    } else {
      col++;
    }
  }

  updateScrollLimit(col, row + scrollOffset);

  // Restore viewport
  display.setViewport(old);
}
