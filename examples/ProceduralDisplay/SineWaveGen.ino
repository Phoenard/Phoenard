/* Generates a whole lot of sine waves */

typedef struct SineInfo {
  int pos;
  int amplitude;
  float time;
  float speed;
  color_t color;
} SineInfo;

void sineWaveGen() {
  const int sineWaveCnt = 6;
  SineInfo sineWaves[sineWaveCnt];
  for (int d = 0; d < sineWaveCnt; d++) {
    SineInfo &i = sineWaves[d];
    i.amplitude = random(5, TERRAIN_LENGTH / 2);
    i.speed = randomFloat(0.004F, 0.02F);
    i.pos = TERRAIN_MID + randomPosNeg(50);
    i.color = random(0xFFFF);
    i.time = 0.0F;
  }

  color_t colorA = BLACK;
  color_t colorB = random(0xFFFF);
  float colorF = 0.0F;
  for (int progress = 0; progress < SECTION_LENGTH; progress++) {
    fillLine(PHNDisplayHW::colorLerp(colorA, colorB, colorF));
    
    colorF += 0.005F;
    if (colorF >= 1.0F) {
      colorF = 0.0F;
      colorA = colorB;
      if (progress < (SECTION_LENGTH-500)) {
        colorB = random(0xFFFF);
      } else {
        colorB = BLACK;
      }
    }
    
    for (int i = 0; i < sineWaveCnt; i++) {
      sineWaves[i].time += sineWaves[i].speed;
      int amp = sineWaves[i].amplitude * sin(sineWaves[i].time);
      fillHole(sineWaves[i].pos + amp, 14, sineWaves[i].color);
    }
    render();
  }
}
