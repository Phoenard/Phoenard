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

/** @file
@brief Holds the PHN_Widget and PHN_WidgetContainer classes used for managing widgets.
*/

#ifndef _PHN_WIDGET_H_
#define _PHN_WIDGET_H_

#include "PHNDisplayHardware.h"
#include "PHNPalette.h"
#include "PHNTextContainer.h"

/** \name Widget color indices
 * @brief Widget color palette index constants for styling widgets
 */
//@{
#define BACKGROUND  0
#define FRAME       1
#define FOREGROUND  2
#define CONTENT     3
#define HIGHLIGHT   4
#define ACTIVATED   5
//@}

// (background, frame, foreground, content, highlight, activated)
//! A palette instance storing the default colors of a widget
#define WIDGET_DEFAULT_COLORS PALETTE(BLACK, RED, WHITE, BLACK, YELLOW, GRAY_LIGHT)

// Forward-declare the Widget for in the container
class PHN_Widget;

/**
 * @brief A container for storing multiple PHN_Widget instances
 * 
 * The adding and removing of widgets is handled in such a way that they are properly (re)drawn.
 * Removing a widget fills its contents with the background color set to erase it properly.
 * Switching between what widgets are displayed is made easier this way. It is a more efficient
 * alternative to filling the entire screen with a color and redrawing the other widgets.
 * 
 * A widget itself can also hold multiple child-widgets, which are drawn and updated with the
 * parent widget. If you, for example, need to combine multiple widgets together into a single
 * user control, such as a keyboard, this can be done that way.
 *
 * Make sure to set the bounds right after constructing a widget. It is best to set the colors
 * afterwards (unless defaults are used) and then any other widget properties that have to be set.
 * Please read the documentation/examples of a widget before use to find out which functions
 * have to be called before use. Rule of thumb: widgets that have a range or dimensions require
 * these functions to be called before it can be used properly.
 */
class PHN_WidgetContainer {
 public:
  //! @brief Constructor for a new widget container
  PHN_WidgetContainer(void);

  /** @brief Updates all the widgets contained
   *
   * @param[in] update Whether to logic-update the widgets
   * @param[in] draw Whether to draw invalidated widgets
   * @param[in] forceRedraw Whether to force draw the widgets
   */
  void updateWidgets(bool update, bool draw, bool forceRedraw);
  /** @brief Adds a single widget
   *
   * The widget is not drawn until the root display is updated.
   */
  void addWidget(PHN_Widget &widget);
  /** @brief Removes a single widget
   *
   * The removed widget is not un-drawn until the root display is updated.
   */
  void removeWidget(PHN_Widget &widget);
  /** @brief Removes all contained widgets
   *
   * The removed widgets are not un-drawn until the root display is updated.
   */
  void clearWidgets();
  /// Gets the widget stored at an index
  PHN_Widget *widget(int index) { return widget_values[index]; }
  /// Gets how many widgets are stored
  const int widgetCount() { return widget_count; }
 protected:
  /**
   * @brief Removes all child widgets from this container without handling proper redrawing
   *
   * Use this when the container has to be deleted from memory.
   * After silently clearing the container is not to be used again.
   */
  void clearSilent();
  /**
   * @brief Resizes the internal array of widgets to a new capacity
   *
   * Widgets falling outside the range are undrawn and deleted (if specified)
   */
  void setWidgetCapacity(int capacity);

  /// Sets whether added widgets are deleted (were added with new)
  bool deleteAddedWidgets;
 private:
  PHN_Widget **widget_values;
  int widget_count;
};

/**
@brief A single widget which covers a rectangular area of the screen

The widget is the base class for other (user-defined) widgets, which can be added to
the @link PHN_Display display @endlink or to other widgets to be automatically updated and drawn. Inside the
virtual update and draw functions the implementer can define what the widget displays
and does with user input.

In addition the widget stores a color palette for use when drawing, so that users
can easily change the looks of a widget. It also contains several utility functions
to aid in drawing widgets and handling user input.

Drawing the entire widget should be performed inside the draw() function. To tell the
display to redraw your widget, call invalidate() to mark the widget ready for drawing.
This separation of logic and drawing makes it easier to work with widgets in general.
It prevents the drawing of contents that are outdated and it simplifies drawing a widget
when it is first added to the display.
*/
class PHN_Widget : public PHN_WidgetContainer {
   friend class PHN_WidgetContainer;

 public:
  //! @brief Constructor for a new widget, initializing all fields to the default values
  PHN_Widget();
  //! @brief Destructor for a widget, clearing any child widgets and freeing memory
  virtual ~PHN_Widget();
  //! @brief Gets the x-coordinate of the widget
  const int getX(void) { return x; }
  //! @brief Gets the y-coordinate of the widget
  const int getY(void) { return y; }
  //! @brief Gets the width of the widget
  const int getWidth(void) { return width; }
  //! @brief Gets the height of the widget
  const int getHeight(void) { return height; }
  //! @brief Sets the new bounds (x, y, width, height) of the widget
  void setBounds(int x, int y, int width, int height);
  //! @brief Sets the size (width, height) of the widget
  void setSize(int width, int height);
  /** @brief Sets a style color for this widget
   * 
   * The colorId specifies an index in the palette to set the color.
   * Standard indices for all widgets are defined as the constants
   * BACKGROUND, FRAME, FOREGROUND, CONTENT, HIGHLIGHT and ACTIVATED.
   */
  virtual void setColor(int colorId, color_t color);
  /** @brief Gets a style color set for this widget
   * 
   * The colorId specifies an index in the palette to set the color.
   * Standard indices for all widgets are defined as the constants
   * BACKGROUND, FRAME, FOREGROUND, CONTENT, HIGHLIGHT and ACTIVATED.
   */
  const color_t color(int colorId) { return colors.get(colorId); }

  //! @brief Gets whether the widget is touched down by the user
  bool isTouched();
  //! @brief Gets whether the user's touch input entered the widget
  bool isTouchEnter();
  //! @brief Gets whether the user's touch input left the widget
  bool isTouchLeave();
  //! @brief Gets whether the user clicked, entered or left the widget
  bool isTouchChange();
  //! @brief Gets whether the user clicked (pressed and released) the widget
  bool isClicked();
  
  //! @brief Fills the area of this widget with a color
  void fillWidgetArea(color_t color);
  //! @brief Sets whether the widget is displayed and updated
  void setVisible(bool visible);
  //! @brief Gets whether the widget is displayed and updated
  bool isVisible(void);
  //! @brief Invalidated the widget, causing it to be re-drawn at a later time
  void invalidate(void);
  //! @brief Checks whether the widget is invalidated and needs to be redrawn
  bool isInvalidated(void);
  //! @brief Draws the widget if invalidated, clearing the invalidated state
  void draw_validate();
  //! @brief Draw function routine, where widgets must perform drawing
  virtual void draw(void) = 0;
  //! @brief Update function routine, where widget logic must be performed
  virtual void update(void) = 0;
protected:
  //! \name Widget bounds
  //@{
  int x, y, width, height;
  //@}
  //! A palette of colors set for the widget
  PHN_Palette colors;
  //! Drawn state of the widget
  bool drawn;
  //! Invalidated state of the widget
  bool invalidated;
  //! Visibility state of the widget
  bool visible;
};

#endif