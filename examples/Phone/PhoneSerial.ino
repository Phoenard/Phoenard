/* Allows the user to enter AT Commands manually or through the computer */

void showSerial() {
  // Disable automatic SIM updates as they would influence
  header.setUpdating(false);
  header.setNavigation(" Back ", "Clear");
  display.fill(BLACK);

  // Textbox for showing output from the last command
  PHN_TextBox outputBox;
  outputBox.setBounds(7, 5, 305, 60);
  outputBox.setTextSize(1);
  outputBox.setMaxLength(1000);
  outputBox.showCursor(false);
  outputBox.showScrollbar(true);
  outputBox.setText("Serial Opened\n");
  display.addWidget(outputBox);

  // Textbox for entering commands
  PHN_TextBox inputBox;
  inputBox.setBounds(7, 70, 305, 24);
  inputBox.setTextSize(2);
  inputBox.showBackspace(true);
  inputBox.setText("AT");
  display.addWidget(inputBox);

  // Keyboard for entering commands
  PHN_Keyboard inputKeys;
  inputKeys.setBounds(7, 102, 310, 113);
  inputKeys.setDimension(11, 5);
  inputKeys.setSpacing(2);
  inputKeys.addKeys("ABC", "1234567890+"
                           "QWERTYUIOP="
                           "ASDFGHJKL.,"
                           "\f\fZXCVBNM\n\n"
                           "\r\r\r     \r\r\r");
  inputKeys.addKeys("?!.", "!@#$%^&*()+"
                           "-.,_\\/?~<>="
                           ":;\'\"[]{}|.,"
                            "\f\f!?.,/\\\"\n\n"
                           "\r\r\r     \r\r\r");
  inputKeys.addKeys("abc", "1234567890+"
                           "qwertyuiop="
                           "asdfghjkl.,"
                           "\f\fzxcvbnm\n\n"
                           "\r\r\r     \r\r\r");
  display.addWidget(inputKeys);

  // Transfer between the two serial ports routinely
  do {
    display.update();

    // Handle input from the keyboard
    if (inputKeys.clickedKey() == '\n') {
      Serial1.println(inputBox.text());
      inputBox.setText("");
    } else if (inputKeys.clickedKey()) {
      inputBox.setSelection(inputKeys.clickedKey());
    }
    
    // Read contents from Serial and write it to the SIM Serial
    while (Serial.available()) {
      Serial1.write(Serial.read());
    }
    
    // Read response from SIM Serial and write it
    while (Serial1.available()) {
      char c = Serial1.read();
      Serial.write(c);
      if (outputBox.textLength() >= (outputBox.maxLength()-200)) {
        // Remove the first 200 characters of text
        outputBox.setSelectionRange(0, 200);
        outputBox.setSelection("");
        outputBox.setSelectionRange(outputBox.textLength(), 0);
      }
      outputBox.setSelection(c);
    }
    
    // Clear screen
    if (header.isAccepted()) {
      outputBox.setText("");
    }
  } while (!header.isCancelled());
  
  display.removeWidget(outputBox);
  display.removeWidget(inputBox);
  display.removeWidget(inputKeys);

  // Restore header
  header.setUpdating(true);
}
