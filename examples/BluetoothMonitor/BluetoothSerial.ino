/*
 * This part of the sketch shows a Serial monitor while connected.
 * Most of this is taken over from the 'SerialMonitor' example sketch.
 */

void showSerial() {
  // Clear Serial and initialize at the baud rate set
  delay(50);
  flushRead(Serial2);

  // Label that shows the device address
  PHN_Label addrLabel;
  addrLabel.setBounds(5, 5, 120, 22);
  addrLabel.setDrawFrame(true);
  addrLabel.setColor(BACKGROUND, WHITE);
  addrLabel.setColor(CONTENT, RED);
  addrLabel.setText(moduleAddress);
  display.addWidget(addrLabel);

  // Newline selection widget - 0=CrLf, 1=Cr, 2=Lf, 3=None
  PHN_NumberBox newlineSel;
  newlineSel.setBounds(130, 5, 120, 22);
  newlineSel.setRange(0, 3);
  newlineSel.setValue(0);  // Use CrLf by default
  newlineSel.setText("CrLf");
  display.addWidget(newlineSel);

  // Button used to clear the output text buffer
  PHN_Button clearBtn;
  clearBtn.setBounds(255, 5, 60, 22);
  clearBtn.setText("Clear");
  display.addWidget(clearBtn);

  // Text widget showing the input to be sent to serial
  PHN_TextBox inputText;
  inputText.setBounds(5, 32, 310, 22);
  inputText.setTextSize(2);
  inputText.setMaxLength(200);
  display.addWidget(inputText);

  // Text widget shows all the latest text
  PHN_TextBox outputText;
  outputText.setBounds(5, 60, 310, 60);
  outputText.setMaxLength(1000);
  outputText.setTextSize(1);
  outputText.showScrollbar(true);
  outputText.showCursor(false);
  display.addWidget(outputText);

  // Keyboard widget so the user can enter text to send to Serial
  PHN_Keyboard inputKeys;
  inputKeys.setBounds(7, 124, 310, 113);
  inputKeys.setDimension(11, 5);
  inputKeys.setSpacing(2);
  inputKeys.addKeys("abc", "1234567890."
                           "qwertyuiop,"
                           "asdfghjkl\b\b"
                           "\f\fzxcvbnm\n\n"
                           "\r\r       \r\r");
  inputKeys.addKeys("ABC", "1234567890."
                           "QWERTYUIOP,"
                           "ASDFGHJKL\b\b"
                           "\f\fZXCVBNM\n\n"
                           "\r\r       \r\r");
  inputKeys.addKeys("?!.", "!@#$%^&*()."
                           "-+_=\\/?~<>,"
                           ":;\'\"[]{}|\b\b"
                           "\f\f!?.,/\\\"\n\n"
                           "\r\r       \r\r");
  display.addWidget(inputKeys);
  
  
  while (true) {
    display.update();
  
    // Update newline selection widget
    if (newlineSel.isValueChanged()) {
      const char* text[] = {"CrLf", "Cr", "Lf", "None"};
      newlineSel.setText(text[newlineSel.value()]);
    }
  
    // Clear output text when clicked
    if (clearBtn.isClicked()) {
      outputText.setText("");
    }
  
    // Update output received
    int outputLimit = (outputText.maxLength() * 0.8);
    while (Serial2.available()) {
      outputText.setSelection(Serial2.read());
    
      // Limit length, remove from start if exceeding
      if (outputText.textLength() > outputLimit) {
        outputText.setSelectionRange(0, outputText.textLength()-outputLimit);
        outputText.setSelection("");
        outputText.setSelectionRange(outputText.textLength(), 0);
      }
    }

    // Handle key input
    char clickedKey = inputKeys.clickedKey();
    if (clickedKey) {
      // Confirm with a newline (ENTER), otherwise append to input text
      if (clickedKey == '\n') {
        const char* newlines[] = {"\r\n", "\r", "\n", ""};
        Serial2.print(inputText.text());
        Serial2.print(newlines[newlineSel.value()]);
        inputText.clearText();
      } else {
        inputText.setSelection(clickedKey);
      }
    }
  }
}
