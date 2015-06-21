/*
 * This part of the sketch stores useful draw functions to make procedurally
 * generated worlds simpler. When you are done generating a line, simply
 * call render() to complete the scene.
 */

/* Fills the full line with a certain color */
void fillLine(color_t color) {
  for (int i = 0; i < TERRAIN_LENGTH; i++) {
    terrainBuffer[i] = color;
  }
  //memset(terrainBuffer, color, sizeof(terrainBuffer));
}

/* Sets a single pixel to a certain color */
void setSingle(int x, color_t color) {
  if (x >= 0 && x < TERRAIN_LENGTH) {
    terrainBuffer[x] = color;
  }
}

/* Sets a range of a line to a color */
void fillRange(int x, int width, color_t color) {
  if (x >= TERRAIN_LENGTH) return;
  if (x < 0) {
    width += x;
    x = 0;
  }
  if (width <= 0) return;
  if ((x + width) >= TERRAIN_LENGTH) {
    width = TERRAIN_LENGTH - x - 1;
  }
  while (width--) {
    terrainBuffer[x + width] = color;
  }
}

/* Fills a part of the line with a color, x being center */
void fillHole(int x, int width, color_t color) {
  int width_half = (width >> 1);
  fillRange(x - (width >> 1), width, color);
}

/* Fills the outer edges of the line with a color */
void fillEdges(int width, color_t color) {
  if (width < TERRAIN_LENGTH && width > 0) {
    while (width--) {
      terrainBuffer[width] = color;
      terrainBuffer[TERRAIN_LENGTH-width-1] = color;
    }
  }
}

/* Renders a tunnel of a certain length, changing from one hole to another */
void renderTunnel(int startHoleX, int startHoleWidth, int endHoleX, int endHoleWidth, int length, color_t colorA, color_t colorB) {
  int holeX, holeW;
  for (int i = 0; i < length; i++) {
    holeX = map(i, 0, length, startHoleX, endHoleX);
    holeW = map(i, 0, length, startHoleWidth, endHoleWidth);
    fillLine(colorB);
    fillHole(holeX, holeW, colorA);
    render();
  }
}

/* Return True for a given % of the time */
boolean chance(int percentChance) {
  return random(100) < percentChance;
}

/* Returns a value +- a certain range */
int randomPosNeg(int range) {
  return random((range << 1) + 1) - range;
}

/* Returns a random float between a range of values */
float randomFloat(float minValue, float maxValue) {
  float f = (float) random(0xFFFF) / (float) 0xFFFF;
  return f * minValue + (1.0F - f) * maxValue;
}
