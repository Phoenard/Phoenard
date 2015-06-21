/* Generates a tunnel with random specs stored within */

void tunnelGen() {
  int tunnelDirection = 0;
  int tunnelCtr = 0;
  int trackPos = TERRAIN_MID;
  int holeWidth = 50;
  color_t colorA = WHITE;
  color_t colorB = WHITE;
  color_t color = colorA;
  float colorF = 0.0F;
  
  // Random dots
  int dotPos;
  color_t dotColor;
  int dotWidth;
  int dotWidthRem = 0;

  renderTunnel(TERRAIN_MID, TERRAIN_LENGTH, trackPos, holeWidth, 50, BLACK, colorA);

  for (int progress = 0; progress < SECTION_LENGTH; progress++) {
    color = PHNDisplayHW::colorLerp(colorA, colorB, colorF);
    fillLine(color);
    
    colorF += 0.01;
    if (colorF >= 1.0F) {
      colorF = 0.0F;
      colorA = colorB;
      
      uint8_t r = random(0xFF);
      colorB = PHNDisplayHW::color565(r, r, r);
    }
        
    if (chance(5)) {
      if (trackPos >= (0.9 * TERRAIN_LENGTH)) {
        tunnelDirection = -random(4);
      } else if (trackPos <= (0.1 * TERRAIN_LENGTH)) {
        tunnelDirection = random(4);
      } else {
        tunnelDirection = randomPosNeg(4);
      }
    }
    trackPos += randomPosNeg(2);

    tunnelCtr++;
    if (tunnelCtr >= abs(tunnelDirection)) {
      tunnelCtr = 0;
      if (tunnelDirection > 0) {
        trackPos++;
      } else if (tunnelDirection < 0) {
        trackPos--;
      }
    }

    if (dotWidthRem) {
      dotWidthRem--;
      fillHole(dotPos, dotWidth, dotColor);
    } else if (chance(8)) {
      dotPos = random(TERRAIN_LENGTH);
      dotWidth = random(8);
      dotWidthRem = dotWidth;
      dotColor = random(0xFFFF);
    }

    fillHole(trackPos, holeWidth, BLACK);
        
    render();
  }
  
  renderTunnel(trackPos, holeWidth, TERRAIN_MID, TERRAIN_LENGTH, 50, BLACK, color);
}
