/*
 * Types Serial input into a textbox widget
 */
#include "Phoenard.h"

// Define the textbox widget
PHN_TextBox text;

void setup() {
  // Open the Serial port
  Serial.begin(9600);
  
  // Set up and add the textbox widget
  text.setBounds(10, 10, 300, 220);
  text.setTextSize(2);
  text.setText("Sample text. Type in console to interact");
  display.addWidget(text);
}

void loop() {
  // When text is received, write it to the text field
  if (Serial.available()) {
    text.setSelection(Serial.read());
  }
  // Regularly update
  display.update();
}
