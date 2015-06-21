/* Generates a bunch of bouncing lines, as if raining colorful pixels! */

typedef struct BouncingLineInfo {
  int decline;
  int declineCtr;
  int fact;
  int position;
  color_t color;
} BouncingLineInfo;

void bouncingLineGen() {
  const int LINE_COUNT = 10;
  BouncingLineInfo lines[LINE_COUNT];
  for (int i = 0; i < LINE_COUNT; i++) {
    lines[i].position = 0;
    lines[i].color = random(0xFFFF);
    lines[i].decline = 0;
    lines[i].declineCtr = 0;
    lines[i].fact = 3;
  }
  
  for (int progress = 0; progress < SECTION_LENGTH; progress++) {
    float bright;
    if (progress < (SECTION_LENGTH/2)) {
      bright = (float) progress / (float) (SECTION_LENGTH/2);
    } else {
      bright = (float) (SECTION_LENGTH - progress) / (float) (SECTION_LENGTH/2);
    }
    
    fillLine(PHNDisplayHW::colorLerp(BLACK, WHITE, bright));
    
    for (int i = 0; i < LINE_COUNT; i++) {
      BouncingLineInfo &info = lines[i];
      if (info.position >= 0 && info.position < TERRAIN_LENGTH) {
        for (int i = 0; i < info.fact; i++) {
          if (info.declineCtr > 0) {
            info.declineCtr--;
          } else if (info.declineCtr < 0) {
            info.declineCtr++;
          } else {
            info.declineCtr = info.decline;
            if (info.decline < 0) {
              info.position--;
            } else {
              info.position++;
            }
          }
          fillHole(info.position, 6, info.color);
        }
      } else if (progress < (SECTION_LENGTH-500)) {
        info.fact = 1 + random(6);
        if (info.fact == 1) {
          info.decline = 0;
        } else if (info.decline < 0) {
          info.decline = random(3);
        } else {
          info.decline = -random(3);
        }
        info.declineCtr = info.decline;
        if (info.position < 0) {
          info.position = 0;
        } else {
          info.position = TERRAIN_LENGTH-1;
        }
      }
    }
    
    render();
  }
}
