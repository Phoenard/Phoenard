/* Generates a bunch of planets of varying colors and size */

typedef struct PlanetInfo {
  PlanetInfo(): widthRem(0) {}
  int pos;
  color_t colorA;
  color_t colorB;
  int width;
  int widthRem;
} PlanetInfo;

void planetUpdate(PlanetInfo &info, int chancePer, int sizeMin, int sizeMax);

void planetGen() {
  PlanetInfo rareRoid;
  PlanetInfo commonRoid;
  for (int progress = 0; progress < SECTION_LENGTH; progress++) {
    fillLine(BLACK);
    setSingle(random(TERRAIN_LENGTH), WHITE);
    
    int p_min, p_max;
    if (chance(70)) {
      p_min = 20;
      p_max = 100;
    } else {
      p_min = 150;
      p_max = 300;
    }
    
    planetUpdate(commonRoid, 20, 1, 15);
    planetUpdate(rareRoid, progress <= (SECTION_LENGTH-300), p_min, p_max);

    render();
  }
}

void planetUpdate(PlanetInfo &info, int chancePer, int sizeMin, int sizeMax) {
  if (info.widthRem) {
    info.widthRem--;

    // Turn left-right motion into an angle
    float k = (float) info.widthRem / (float) info.width;
    color_t color = PHNDisplayHW::colorLerp(info.colorA, info.colorB, k);
    k = -1.0 + (k * 2.0);
    fillHole(info.pos, (int) (info.width * sin(acos(k))), color);
  } else if (chance(chancePer)) {
    info.pos = random(TERRAIN_LENGTH);
    info.width = random(sizeMin, sizeMax);
    info.widthRem = info.width;
    info.colorA = random(0xFFFF);
    info.colorB = PHNDisplayHW::colorLerp(BLACK, random(0xFFFF), 0.1);
  }
}
