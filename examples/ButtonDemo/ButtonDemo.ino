/*
 * Demo for button functionality. The screen shortly flashes with a color
 * as the corresponding button is pressed.
 */
#include "Phoenard.h"

// Define four buttons
PHN_Button button_white;
PHN_Button button_red;
PHN_Button button_green;
PHN_Button button_blue;

void setup() {
  // Set up the white button
  button_white.setBounds(10, 10, 100, 40);
  button_white.setColor(FOREGROUND, WHITE);
  button_white.setText("Flash");
  display.addWidget(button_white);
  
  // Set up the red button
  button_red.setBounds(10, 60, 100, 40);
  button_red.setColor(FOREGROUND, RED);
  button_red.setText("Flash");
  display.addWidget(button_red);
  
  // Set up the green button
  button_green.setBounds(10, 110, 100, 40);
  button_green.setColor(FOREGROUND, GREEN);
  button_green.setText("Flash");
  display.addWidget(button_green);
  
  // Set up the blue button
  button_blue.setBounds(10, 160, 100, 40);
  button_blue.setColor(FOREGROUND, BLUE);
  button_blue.setText("Flash");
  display.addWidget(button_blue);
}

void loop() {
  // Update the display to refresh buttons
  display.update();
  
  // Handle button clicks, flashing the screen for each color
  if (button_white.isClicked()) {
    flash(WHITE);
  }
  if (button_red.isClicked()) {
    flash(RED);
  }
  if (button_green.isClicked()) {
    flash(GREEN);
  }
  if (button_blue.isClicked()) {
    flash(BLUE);
  }
}

void flash(color_t color) {
  // Flash a rectangular screen with a color, and then back to black after a delay
  display.fillRect(120, 0, display.width() - 120, display.height(), color);
  delay(50);
  display.fillRect(120, 0, display.width() - 120, display.height(), BLACK);
}