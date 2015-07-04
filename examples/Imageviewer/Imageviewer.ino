/*
 * Displays .BMP and .LCD images saved inside the Images subdirectory
 */
#include "Phoenard.h"
#include <SD.h>

void setup() {
  // Attempt to initialize the SD card
  if (!SD.begin(SD_CS_PIN)) {
    display.debugPrint(50, 50, 3, "No Micro-SD");
    for (;;);
  }
}

void loop() {
  // Repeatedly show all images found in /Images
  ShowImages(SD.open("/Images"));
}

void ShowImages(File dir) {
  // Go through all files and subdirectories
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;
    
    char* name = entry.name();
    char* ext = name + strlen(name) - 3;

    if (entry.isDirectory()) {
      ShowImages(entry);
    } else if (!memcmp(ext, "BMP", 3) || !memcmp(ext, "LCD", 3)) {

      // Display the image and wait for the touchscreen to be clicked
      display.fill(BLACK);
      display.drawImage(entry, 0, 0);
      do {
        display.update();
      } while (!display.isTouchUp());
    }
    entry.close();
  }
}
