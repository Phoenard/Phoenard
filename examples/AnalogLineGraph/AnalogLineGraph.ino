/*
 * Displays a running line graph of the first 3 analog inputs
 */
#include "Phoenard.h"

// Define the line graph
PHN_LineGraph graph;

void setup() {
  graph.setBounds(0, 0, 320, 240); // Set the bounds of the line graph
  graph.setRange(0.0F, 1023.0F);   // Set the minimum/maximum value range
  graph.setLineCount(2);           // Set how many lines are used
  graph.setLineColor(0, RED);      // Set the line color of the first line
  graph.setLineColor(1, BLUE);     // Set the line color of the second line
  graph.setAutoClear(true);        // Set whether the screen is fully cleared each turn (default=true)
  display.addWidget(graph);        // Add the line graph widget
}

void loop() {
  // Update the display (needed to draw line graph frame)
  display.update();

  // Create an array of values to add to the graph
  float values[2];
  values[0] = analogRead(A0);
  values[1] = analogRead(A1);

  // Add the values to the graph
  graph.addValues(values);
}
