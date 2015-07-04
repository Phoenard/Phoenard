/* 
 * Shows an image sliding through the screen - store slides in /Slides folder
 * Make sure the images are vertically-aligned and stored in LCD format!
 */

#include "SD.h"

File slideFolder;
boolean sd_init = false;

void slideshowGen() {
  // Initialize the Micro-SD and find/rewind the slide folder
  if (!sd_init && !SD.begin(SD_CS_PIN)) {
    return;
  }
  sd_init = true;
  if (!slideFolder) {
    slideFolder = SD.open("/SLIDES");
    if (!slideFolder) {
      return;
    }
  }

  // Open next file
  File f = slideFolder.openNextFile();
  if (!f) {
      slideFolder.rewindDirectory();
      f = slideFolder.openNextFile();
      if (!f) {
        return; // Empty folder
      }
  }
  
  // Skip if not a LCD format file
  if ((f.read() != 'L') || (f.read() != 'C') || (f.read() != 'D')) {
    return;
  }

  Serial.begin(9600);
  Serial.print("RENDERING ");
  Serial.println(f.name());

  // Read the header of the file
  Imageheader_LCD header;
  f.readBytes((char*) &header, sizeof(header));

  Serial.print("WIDTH: ");
  Serial.println(header.width);
  Serial.print("HEIGHT: ");
  Serial.println(header.height);
  Serial.print("COLORS: ");
  Serial.println(header.colors);

  // Read colormap, empty if not used/available
  color_t colorMap[header.colors];
  uint16_t ci;
  color_t c565;
  for (ci = 0; ci < header.colors; ci++) {
    f.readBytes((char*) &c565, sizeof(c565));
    colorMap[ci] = c565;
  }

  // Clear buffer first
  fillLine(BLACK);

  // Write pixels to screen
  uint16_t x, y;
  if (header.bpp == 16) {
    // Fast method
    for (y = 0; y < header.height; y++) {
      for (x = 0; x < header.width; x++) {
        f.readBytes((char*) &c565, sizeof(c565));
        setSingle(TERRAIN_LENGTH - x, c565);
      }
      render();
    }
  } else {
    int tmpbuff = 0;
    int tmpbuff_len = 0;
    int pixelmask = (1 << header.bpp) - 1;
    for (y = 0; y < header.height; y++) {
      for (x = 0; x < header.width; x++) {
        if (!tmpbuff_len) {
          tmpbuff = (f.read() & 0xFF);
          tmpbuff_len = 8;
        }
        setSingle(TERRAIN_LENGTH - x, colorMap[tmpbuff & pixelmask]);
        tmpbuff >>= header.bpp;
        tmpbuff_len -= header.bpp;
      }
      render();
    }
  }

  /* Wait a moment */
  delay(1500);
}


























