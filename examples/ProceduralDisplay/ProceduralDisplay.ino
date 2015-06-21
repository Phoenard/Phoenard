/*
 * Procedurally generates terrain line-by-line on the screen using algorithms.
 * Build amazing sidescrolling action with clever mathematic formulas, or render
 * a cool sine wave. All up to you!
 *
 * Add your 'effect' function to the functions array. Inside your function, after
 * having generated a new line, call render() to render it to the screen. The
 * render function takes care of frame delays.
 */
#include "Phoenard.h"

#define TERRAIN_LENGTH 240   // Length of the terrain (height of the screen)
#define TERRAIN_DELAY  1000  // Delay between each update that is enforced (in microseconds)
#define TERRAIN_OFFSET 10    // Offset needed to prevent pixel overflow
#define TERRAIN_MID    (TERRAIN_LENGTH/2) // Middle of the terrain
#define TERRAIN_GAP    40    // Pixel gap between each terrain generated
#define SECTION_LENGTH 4000  // The length of a single animated section

/* A list of functions the renderer will go through */
void (*functions[])() = {planetGen, tunnelGen, bouncingLineGen, sineWaveGen};

color_t terrainBuffer[TERRAIN_LENGTH]; // Buffer of the current terrain line being generated
int terrainPosition = 0;               // Current scrolling position on the screen
unsigned long terrainLastDraw = 0;     // Time stamp of last terrain line drawn

void setup() {
  // Run a main rendering/updating loop here
  for (;;) {
    for (int i = 0; i < (sizeof(functions) / sizeof(functions[0])); i++) {
      randomSeed(analogRead(0));
      functions[i]();
      
      // Render a gap
      int gap = TERRAIN_GAP;
      while (gap--) {
        fillLine(BLACK);
        render();
      }
    }
  }
}

void loop() {
  // Never executed
}

/* Renders the current terrain line to the screen */
void render() {
  // Wait until the draw delay has passed
  while ((micros() - terrainLastDraw) < TERRAIN_DELAY);
  
  // Scroll the screen one pixel 'down'
  terrainPosition--;
  if (terrainPosition < 0) terrainPosition = 319;
  PHNDisplayHW::writeRegister(LCD_CMD_GATE_SCAN_CTRL3, terrainPosition);

  // Wipe the line of pixels at the end of the terrain
  updateScreenCursor(0, 0);
  PHNDisplay8Bit::writePixels(BLACK_8BIT, 240);

  // Draw the buffered line to screen
  updateScreenCursor(0, TERRAIN_OFFSET);
  PHNDisplay16Bit::writePixels(terrainBuffer, 240);
  
  // Done - update start time
  terrainLastDraw = micros();
}

/* Moves the cursor to a given position on the screen taking scrolling into account */
void updateScreenCursor(int x, int y) {
  int line = (320-y) - terrainPosition;
  if (line < 0) line += 320;
  if (line >= 320) line -= 320;
  PHNDisplayHW::setCursor(line, x, DIR_DOWN);
}
