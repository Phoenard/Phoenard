/*
 * This part of the sketch contains all the functions for handling the calling logic
 * If from somewhere in software we need to call someone or something, or need to
 * accept a call, a function can be found here to do so.
 */

void startCall() {
  startCall("");
}

void startCall(const char* numberToCall) {
  char number[20];
  strcpy(number, numberToCall);
  if (sim.isCalled()) {
    // Receive the call
    strcpy(number, sim.getIncomingNumber());
    handleCall(number, true, true);
  } else {
    // Start a new call
    handleCall(number, false, true);
  }
}

boolean enterNumber(char* number) {
  number[0] = 0;
  handleCall(number, false, false);
  return number[0] != 0;
}

void endCall() {
  // End an ongoing call
  sim.endCall();
}

/*
 * Generic function that shows a keypad for entering a number, as well
 * handling the picking up or initiating a call.
 * The input number array is updated when entered by the user.
 */
void handleCall(char* number, boolean receiving, boolean startCall) {
  // Keyboard with number layout for entering a number
  PHN_Keyboard numberKeys;
  numberKeys.setBounds(65, 60, 180, 132);
  numberKeys.setSpacing(4, 4);
  numberKeys.setDimension(3, 4);
  numberKeys.setKeys("123456789*0#");
  numberKeys.setColor(FOREGROUND, BUTTON_COLOR);
  numberKeys.setColor(BACKGROUND, GRAY_LIGHT);
  display.addWidget(numberKeys);

  // Fill background below keyboard with gray
  display.fillRect(1, numberKeys.getY() - 5, 318, numberKeys.getHeight() + 10, GRAY_LIGHT);

  // Track whether we should stop
  boolean aborted = false;

  // If not calling, enter a number first
  if ((number[0] == 0) && !receiving) {
    // Set navigation to these options
    header.setNavigation("Cancel", startCall ? " Call " : " Next ");

    // Text box to show the number entered by the user
    PHN_TextBox numberText;
    numberText.setBounds(15, 25, 290, 22);
    numberText.setTextSize(2);
    numberText.setMaxLength(15);
    numberText.showBackspace(true);
    numberText.setText("");
    display.addWidget(numberText);

    // Routinely update and enter inputed information
    do {
      display.update();
      
      if (numberKeys.clickedKey()) {
        numberText.setSelection(numberKeys.clickedKey());
      }
    } while (!header.isNavigating());

    // Remove the number input widgets
    display.removeWidget(numberText);

    // If not accepted or not calling, stop here
    if (header.isCancelled()) {
      aborted = true;
    } else {
      // Update number
      strcpy(number, numberText.text());
    }
  }
  if (!startCall) {
    aborted = true;
  }

  if (!aborted) {
    // Set navigation to only cancel a call
    header.setNavigation("End Call", NULL);
    display.update();

    // Show the number that is being called
    String numberText;
    numberText += "Calling with ";
    numberText += number;
    PHN_Label numberLabel;
    numberLabel.setBounds(5, 15, 310, 40);
    numberLabel.setText(numberText);
    display.addWidget(numberLabel);

    // Accept or initiate the call on the SIM
    if (receiving) {
      sim.acceptCall();
    } else {
      sim.call(number);
    }

    int callStatus;
    boolean isHangUp = false;
    while ((callStatus = sim.getCallStatus()) == SIM_CALL_STATUS_CALLING) {
      // Update widgets
      display.update();

      // Send entered keypad input to the SIM
      sim.sendDTMF(numberKeys.clickedKey());

      // Cancel the call if requested so
      if (header.isCancelled()) {
        endCall();
        isHangUp = true;
      }
    }

    // Show what happened to the call shortly
    if (callStatus == SIM_CALL_STATUS_BUSY) {
      header.showStatus("Caller is busy");
    }
    if (!isHangUp && (callStatus == SIM_CALL_STATUS_NONE)) {
      header.showStatus("Call ended.");
    }

    // Remove widgets and done
    display.removeWidget(numberLabel);
  }

  // Clean up the screen and remove widgets
  display.removeWidget(numberKeys);
  display.update();
  display.fillRect(1, numberKeys.getY() - 5, 318, numberKeys.getHeight() + 10, BLACK);
}
