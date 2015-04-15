/*
 * Display the 6 analog input voltage levels on gauge widgets
 */
#include "Phoenard.h"

// Define 6 gauge widgets
PHN_Gauge gauge[6];

// Define the foreground colors to be used
color_t gauge_colors[6] = {RED, GREEN, BLUE, ORANGE, YELLOW, WHITE};

void setup() {
  // Set up and add all 6 widgets in a 3x2 grid
  for (int i = 0; i < 3; i++) {
    gauge[i].setBounds(20 + i * 93, 20, 80, 80);
    gauge[i].setRange(0.0F, 3.3F);
    gauge[i].setColor(FOREGROUND, gauge_colors[i]);
    display.addWidget(gauge[i]);
  }
  for (int i = 0; i < 3; i++) {
    gauge[i + 3].setBounds(20 + i * 93, 120, 80, 80);
    gauge[i + 3].setRange(0.0F, 3.3F);
    gauge[i + 3].setColor(FOREGROUND, gauge_colors[i + 3]);
    display.addWidget(gauge[i + 3]);
  }
}

void loop() {
  // Update the values for all widgets
  for (int i = 0; i < 6; i++) {
    gauge[i].setValue((float) analogRead(i) / 1023.0F * 3.3F);
  }
  // Update the display to refresh the gauges
  display.update();
  // Short delay to reduce screen flicker
  delay(20);
}