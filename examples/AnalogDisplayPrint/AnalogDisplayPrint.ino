/*
 * Makes use of display debug print functions to draw the ADC and voltage levels of the 6 analog inputs
 */
#include "Phoenard.h"

#define SPACING 35  // Spacing between rows
#define TEXTSIZE 2  // Size of text drawn

void setup() {
  // For each analog input print labels
  char* names[6] = {"A0:", "A1:", "A2:", "A3:", "A4:", "A5:"};
  for (int i = 0; i < 6; i++) {
    // Display using the text debug print function
    display.debugPrint(10, 10 + SPACING * i, TEXTSIZE, names[i]);
    display.debugPrint(60, 10 + SPACING * i, TEXTSIZE, "ADC=");
    display.debugPrint(190, 10 + SPACING * i, TEXTSIZE, "Volt=");
  }
}

void loop() {
  // For each analog input print the raw analog values and estimated voltage
  for (int i = 0; i < 6; i++) {
    int rawValue = analogRead(i);                           // Read the analog value
    float volt = (float) rawValue / 1023.0F * 3.3F;         // Convert the raw value into an estimated voltage
    
    display.debugPrint(110, 10 + SPACING * i, TEXTSIZE, rawValue);  // Display using the int debug print function
    display.debugPrint(250, 10 + SPACING * i, TEXTSIZE, volt);      // Display using the float debug print function
  }
}