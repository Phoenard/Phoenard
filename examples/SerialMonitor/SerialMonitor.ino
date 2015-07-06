/*
 * Shows a keyboard and input/output text for a Serial port
 * This sketch allows you to communicate with Serial devices without needing a computer terminal
 * For example, it can be used to debug Serial devices connected to rx/tx, such as XBee
 * You can change the Serial used in the definition at the top
 */
#include "Phoenard.h"

// Defines the Serial communicated with
#define SERIAL_USED Serial

// Serial BAUD rates that are selectable
const long baud_rates[] = {300, 1200, 2400, 4800, 9600, 14400,
                           19200, 28800, 38400, 57600, 115200};

// Widgets
PHN_NumberBox baudSel;
PHN_NumberBox newlineSel;
PHN_Button    clearBtn;
PHN_TextBox   inputText;
PHN_TextBox   outputText;
PHN_Keyboard  inputKeys;

void setup() {
  // Baud selection widget, to setup the baud rate used
  baudSel.setBounds(5, 5, 120, 22);
  baudSel.setRange(0, (sizeof(baud_rates) / sizeof(long)) - 1);
  baudSel.setValue(4);  // Use 9600 by default
  baudSel.setText(9600);
  display.addWidget(baudSel);

  // Newline selection widget - 0=CrLf, 1=Cr, 2=Lf, 3=None
  newlineSel.setBounds(130, 5, 120, 22);
  newlineSel.setRange(0, 3);
  newlineSel.setValue(0);  // Use CrLf by default
  newlineSel.setText("CrLf");
  display.addWidget(newlineSel);

  // Button used to clear the output text buffer
  clearBtn.setBounds(255, 5, 60, 22);
  clearBtn.setText("Clear");
  display.addWidget(clearBtn);

  // Text widget showing the input to be sent to serial
  inputText.setBounds(5, 32, 310, 22);
  inputText.setTextSize(2);
  inputText.setMaxLength(200);
  display.addWidget(inputText);

  // Text widget shows all the latest text
  outputText.setBounds(5, 60, 310, 60);
  outputText.setMaxLength(1000);
  outputText.setTextSize(1);
  outputText.showScrollbar(true);
  outputText.showCursor(false);
  display.addWidget(outputText);

  // Keyboard widget so the user can enter text to send to Serial
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
}

void loop() {
  display.update();

  // Update baud rate used
  if (baudSel.isValueChanged()) {
    long baud = baud_rates[baudSel.value()];
    baudSel.setText(baud);
    SERIAL_USED.begin(baud);
  }
  
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
  while (SERIAL_USED.available()) {
    outputText.setSelection(SERIAL_USED.read());
    
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
      SERIAL_USED.print(inputText.text());
      SERIAL_USED.print(newlines[newlineSel.value()]);
      inputText.clearText();
    } else {
      inputText.setSelection(clickedKey);
    }
  }
}
