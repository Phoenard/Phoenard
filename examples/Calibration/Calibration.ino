/*
 * Calibrates the LCD touchscreen by allowing
 * the user to press 4 white cubes. The raw analog
 * input is used to store the calibration data in
 * EEPROM for use by the library.
 */
#include "Phoenard.h"

typedef struct TouchData {
  int x;
  int y;
};

#define BOX_COLOR_IDLE     WHITE_8BIT   // Color of selectable boxes when not pressed
#define BOX_COLOR_ACTIVE   GREEN_8BIT   // Color of selectable boxes when pressed down
#define BOX_SIZE           32           // Size (width and height) of selectable boxes
#define BOX_EDGE           50           // Edge between each corner of the screen and the boxes

// Pre-defined: prevents compiler issues
TouchData getData(int x, int y);

void setup() {
  // Always draw text with a black background
  display.setTextBackground(BLACK);

  // Instruction text
  display.setCursor(71, 100);
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.print("Press the middle\n"
                "of each cube");

  // Read the four points on the screen
  TouchData data_tl = getData(BOX_EDGE - BOX_SIZE / 2, BOX_EDGE - BOX_SIZE / 2);
  TouchData data_tr = getData(PHNDisplayHW::WIDTH - BOX_EDGE - BOX_SIZE / 2, BOX_EDGE - BOX_SIZE / 2);
  TouchData data_bl = getData(BOX_EDGE - BOX_SIZE / 2, PHNDisplayHW::HEIGHT - BOX_EDGE - BOX_SIZE / 2);
  TouchData data_br = getData(PHNDisplayHW::WIDTH - BOX_EDGE - BOX_SIZE / 2, PHNDisplayHW::HEIGHT - BOX_EDGE - BOX_SIZE / 2);

  // Convert the points into the offset values
  int hor_min = (data_tl.x + data_bl.x) >> 1;
  int hor_max = (data_tr.x + data_br.x) >> 1;
  int ver_min = (data_tl.y + data_tr.y) >> 1;
  int ver_max = (data_bl.y + data_br.y) >> 1;

  // Apply transform logic to make these point to the boundaries of the screen
  int hor_off = (int) (((float) BOX_EDGE / (float) (PHNDisplayHW::WIDTH - (2 * BOX_EDGE))) * (float) (hor_max - hor_min));
  int ver_off = (int) (((float) BOX_EDGE / (float) (PHNDisplayHW::HEIGHT - (2 * BOX_EDGE))) * (float) (ver_max - ver_min));
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
  display.setCursor(47, 70);
  display.setTextColor(GREEN);
  display.setTextSize(2);
  display.print("Screen calibration\n"
                "finished. Press the\n"
                "screen to test the\n"
                "performance. Press\n"
                "RESET to go back.");
}

void loop() {
  // Proceed to run a paint application using touch for debugging purposes
  // Minimalistic, unfiltered touch input is used as opposed to the display library
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
  drawCube(x, y, BLACK_8BIT);

  return touch;
}
