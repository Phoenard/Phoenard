/*
 * A simple Paint sketch - use the touchscreen to paint on the screen
 */
#include "Phoenard.h"

// The size of a color selectable box
#define BOXSIZE 38
// The top-offset of the color selectable boxes
#define BOXOFFSET 10

color_t oldcolor;     // Old color selected, used for detecting color change
color_t currentcolor; // Current color to use for drawing
int box_x;            // X-coordinate of the color selection boxes
int width, height;    // Width and height of the screen
PressPoint last;      // Last point pressed down on the screen
boolean hasLast;      // Whether the use has pressed down before

void setup() {
  hasLast = false;
  width = display.width();
  height = display.height();
  box_x = width - BOXSIZE;
  currentcolor = RED;

  // Draw the color selection area initially
  display.fillRect(box_x, 10, BOXSIZE, BOXSIZE, RED);
  display.fillRect(box_x, 10+BOXSIZE, BOXSIZE, BOXSIZE, YELLOW);
  display.fillRect(box_x, 10+BOXSIZE*2, BOXSIZE, BOXSIZE, GREEN);
  display.fillRect(box_x, 10+BOXSIZE*3, BOXSIZE, BOXSIZE, CYAN);
  display.fillRect(box_x, 10+BOXSIZE*4, BOXSIZE, BOXSIZE, BLUE);
  display.fillRect(box_x, 10+BOXSIZE*5, BOXSIZE, BOXSIZE, MAGENTA);
  display.drawRect(box_x, 10, BOXSIZE, BOXSIZE, WHITE);
}

void loop() {
  display.update();

  // If slider area is touched, wipe the screen
  if (display.isSliderTouched()) {
    display.fillRect(0, 10, box_x, display.height()-10, BLACK);
  }

  if (display.isTouched()) {
    PressPoint p = display.getTouch();

    // Color selection area
    if (p.y > 10 && p.x > box_x) {
      oldcolor = currentcolor;
  
      if (p.y < BOXOFFSET+BOXSIZE*1) { 
        currentcolor = RED; 
        display.drawRect(box_x, BOXOFFSET+BOXSIZE*0, BOXSIZE, BOXSIZE, WHITE);
      } else if (p.y < BOXOFFSET+BOXSIZE*2) {
        currentcolor = YELLOW; 
        display.drawRect(box_x, BOXOFFSET+BOXSIZE*1, BOXSIZE, BOXSIZE, WHITE);
      } else if (p.y < BOXOFFSET+BOXSIZE*3) {
        currentcolor = GREEN; 
        display.drawRect(box_x, BOXOFFSET+BOXSIZE*2, BOXSIZE, BOXSIZE, WHITE);
      } else if (p.y < BOXOFFSET+BOXSIZE*4) {
        currentcolor = CYAN; 
        display.drawRect(box_x, BOXOFFSET+BOXSIZE*3, BOXSIZE, BOXSIZE, WHITE);
      } else if (p.y < BOXOFFSET+BOXSIZE*5) {
        currentcolor = BLUE; 
        display.drawRect(box_x, BOXOFFSET+BOXSIZE*4, BOXSIZE, BOXSIZE, WHITE);
      } else if (p.y < BOXOFFSET+BOXSIZE*6) {
        currentcolor = MAGENTA; 
        display.drawRect(box_x, BOXOFFSET+BOXSIZE*5, BOXSIZE, BOXSIZE, WHITE);
      }
      if (oldcolor != currentcolor) {
        if (oldcolor == RED)     display.fillRect(box_x, BOXOFFSET+BOXSIZE*0, BOXSIZE, BOXSIZE, RED);
        if (oldcolor == YELLOW)  display.fillRect(box_x, BOXOFFSET+BOXSIZE*1, BOXSIZE, BOXSIZE, YELLOW);
        if (oldcolor == GREEN)   display.fillRect(box_x, BOXOFFSET+BOXSIZE*2, BOXSIZE, BOXSIZE, GREEN);
        if (oldcolor == CYAN)    display.fillRect(box_x, BOXOFFSET+BOXSIZE*3, BOXSIZE, BOXSIZE, CYAN);
        if (oldcolor == BLUE)    display.fillRect(box_x, BOXOFFSET+BOXSIZE*4, BOXSIZE, BOXSIZE, BLUE);
        if (oldcolor == MAGENTA) display.fillRect(box_x, BOXOFFSET+BOXSIZE*5, BOXSIZE, BOXSIZE, MAGENTA);
      }
    } else {
      // Connect lines if a previous press-point was set
      // This performs the actual drawing by the user
      if (hasLast) {
        display.drawLine(p.x, p.y, last.x, last.y, currentcolor);
      }
    }
    // Pressed: Store the last point pressed for next time
    last = p;
    hasLast = true;
  } else {
    // Not pressed
    hasLast = false;
  }
}
