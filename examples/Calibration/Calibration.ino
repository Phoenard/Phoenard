/*
 * Calibrates the LCD touchscreen
 */

#include "Phoenard.h"

typedef struct TouchData {
  int x;
  int y;
};

#define LCD_WIDTH   320
#define LCD_HEIGHT  240

#define LCD_MODE_HOR  0x8
#define LCD_MODE_VER  0x0

#define LCD_BLACK     0x00
#define LCD_WHITE     0xFF
#define LCD_YELLOW    0xEE
#define LCD_RED       0xE0
#define LCD_GREEN     0xC7
#define LCD_BLUE      0x9E
#define LCD_PURPLE    0xDC

#define BOX_COLOR_IDLE    LCD_WHITE
#define BOX_COLOR_ACTIVE  LCD_GREEN

#define BOX_SIZE 32
#define BOX_EDGE 50

TouchData getData(int x, int y);

void setup() {
  // Instruction text
  LCD_write_string(36, 100, 2, "Press the middle", LCD_WHITE, LCD_BLACK);
  LCD_write_string(36, 120, 2, "of each cube", LCD_WHITE, LCD_BLACK);

  // Read the four points on the screen
  TouchData data_tl = getData(BOX_EDGE - BOX_SIZE / 2, BOX_EDGE - BOX_SIZE / 2);
  TouchData data_tr = getData(LCD_WIDTH - BOX_EDGE - BOX_SIZE / 2, BOX_EDGE - BOX_SIZE / 2);
  TouchData data_bl = getData(BOX_EDGE - BOX_SIZE / 2, LCD_HEIGHT - BOX_EDGE - BOX_SIZE / 2);
  TouchData data_br = getData(LCD_WIDTH - BOX_EDGE - BOX_SIZE / 2, LCD_HEIGHT - BOX_EDGE - BOX_SIZE / 2);
  
  // Convert the points into the offset values
  int hor_min = (data_tl.x + data_bl.x) >> 1;
  int hor_max = (data_tr.x + data_br.x) >> 1;
  int ver_min = (data_tl.y + data_tr.y) >> 1;
  int ver_max = (data_bl.y + data_br.y) >> 1;
  
  // Apply transform logic to make these point to the boundaries of the screen
  int hor_off = (int) (((float) BOX_EDGE / (float) (LCD_WIDTH - (2 * BOX_EDGE))) * (float) (hor_max - hor_min));
  int ver_off = (int) (((float) BOX_EDGE / (float) (LCD_HEIGHT - (2 * BOX_EDGE))) * (float) (ver_max - ver_min));
  hor_min -= hor_off;
  hor_max += hor_off;
  ver_min -= ver_off;
  ver_max += ver_off;

  // Read settings from EEPROM
  PHN_Settings settings;
  PHN_Settings_Load(settings);

  // Write calibration information to settings
  PHN_Settings_WriteCali(&settings, hor_min, hor_max, ver_min, ver_max);

  // Also set the LOAD flag, to go back to the previous sketch
  settings.flags |= SETTINGS_LOAD;

  // Write new flags to EEPROM
  PHN_Settings_Save(settings);

  // All done, notify user
  LCD_write_string(20, 80,  2, "Screen calibration", LCD_GREEN, LCD_BLACK);
  LCD_write_string(20, 100, 2, "finished. Press the", LCD_GREEN, LCD_BLACK);
  LCD_write_string(20, 120, 2, "screen to test the", LCD_GREEN, LCD_BLACK);
  LCD_write_string(20, 140, 2, "performance. Press", LCD_GREEN, LCD_BLACK);
  LCD_write_string(20, 160, 2, "RESET to go back.", LCD_GREEN, LCD_BLACK);
}

void loop() {
  // Proceed to run a paint application using touch for debugging purposes
  unsigned int x, y;
  float pressure;
  PHNDisplayHW::readTouch(&x, &y, &pressure);

  // Touched?
  if (pressure >= 50.0F) {
    // Draw a pixel there
    PHNDisplayHW::setCursor(x, y, DIR_RIGHT);
    PHNDisplay8Bit::writePixel(WHITE_8BIT);
  }
}

/* Draws a cube of BOXSIZE at the coordinates and using the color specified */
void drawCube(int x, int y, char color) {
  for (char dy = 0; dy < BOX_SIZE; dy++) {
    PHNDisplay8Bit::drawLine(x, y + dy, BOX_SIZE, DIR_RIGHT, color);
  }
}

/* Obtains the touch data for the rectangle drawn at the position specified */
TouchData getData(int x, int y) {
  // Draw the white cube
  drawCube(x, y, BOX_COLOR_IDLE);
  
  // Wait until the touchscreen is pressed down and up again
  boolean pressed = false;
  TouchData touch;
  unsigned int analogX, analogY, analogZ, analogZ2;
  const int Z_THRES_START = 40;
  const int Z_THRES_STOP = 0;
  for (;;) {
    // Track analog x/y
    PHNDisplayHW::readTouch(&analogX, &analogY, &analogZ, &analogZ2);

    // Pressed down firmly - update x/y
    if (analogZ >= Z_THRES_START) {
      // Pressed for the first time? Change cube color
      if (!pressed) {
        pressed = true;
        drawCube(x, y, BOX_COLOR_ACTIVE);
      }

      // Touched - accept the x/y
      touch.x = analogX;
      touch.y = analogY;
    }

    // Stop when no longer pressed
    if (pressed && analogZ <= Z_THRES_STOP) {
      break;
    }
    delay(10);
  }

  // Hide the cube again
  drawCube(x, y, LCD_BLACK);

  return touch;
}
