/*
 * Displays the raw analog input values in 6 bar graph widgets
 */
#include "Phoenard.h"

// Define six bar graphs
PHN_BarGraph bar[6];

// Define six colors to use for the bars
color_t bar_colors[6] = {RED, GREEN, BLUE, ORANGE, YELLOW, BLACK};

void setup() {
  // Set up and add all bar graphs
  for (int i = 0; i < 6; i++) {
    bar[i].setBounds(20 + 45 * i, 20, 40, 200);
    bar[i].setRange(0.0F, 1023.0F);
    bar[i].setColor(CONTENT, bar_colors[i]);
    display.addWidget(bar[i]);
  }
}

void loop() {
  // Update all bar graph values
  for (int i = 0; i < 6; i++) {
    bar[i].setValue(analogRead(i));
  }
  // Refresh the widgets
  display.update();
}
