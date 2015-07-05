/*
 * This is the official Phoenard Arduino Phone OS sketch. The software supports
 * various cool features/operations you can do by default. All functionality
 * is implemented with general-purpose widgets and Phoenard library functions.
 * Feel free to tweak this to your hearts content!
 * -/-
 *
 * The default functions contained within this sketch are:
 * - Powering on/off the SIM908
 * - Checking for and processing SIM PIN/PUK
 * - Live update status of signal, battery and provider
 * - Composing and sending of text messages
 * - Starting and ending voice calls with dial support
 * - Storing and managing contacts / dial logs
 * - Reading and managing inbox
 * - Automatically receive calls/messages (RING)
 * - Realtime SIM Clock with clock adjustment
 * - Scientific calculator
 *
 * This sketch is subject of frequent change.
 * If you encounter a bug, let us know!
 *
 * http://phoenard.com
 */
#include "Phoenard.h"
#include "Icons.c"
#include "PhoneHeader.h"

#include <avr/sleep.h>
#include <avr/power.h>

// Header widget is always visible/in use
PHN_PhoneHeader header;

const color_t BUTTON_COLOR = PHNDisplayHW::color565(255, 240, 100);

void setup() {
  Serial.begin(57600);

  // First turn the sim ON
  if (!sim.isOn()) {
    display.setTextColor(GREEN);
    display.drawStringMiddle(0, 0, 320, 240, "Turning on SIM...");
    sim.begin();
    display.fill(BLACK);
  }

  // Reset the sim to the defaults for operation
  sim.reset();

  // We may accidentally be in a call unable to exit it
  // Always end any ongoing call.
  sim.endCall();

  // Enter sim pin here
  handlePin();

  // Set volume to maximum
  sim.setVolume(100);

  // Set up and add header - is always visible
  display.addWidget(header);
}

void loop() {
  switch (showMainMenu()) {
    case 0:
      startCall();
      break;

    case 1:
      sendMessage();
      break;
    
    case 2:
      showSerial();
      break;

    case 3:
      // Show all last numbers
      showContacts("LA", false);
      break;

    case 4:
      showMessages();
      break;

    case 5:
      // Show the stored contacts list
      showContacts("SM", true);
      break;
      
    case 6:
      showClock();
      break;
      
    case 7:
      showCalculator();
      break;

    case 8:
      shutDown();
      break;
  }
}

int showMainMenu() {
  // Disable the navigation buttons
  header.setNavigation(NULL, NULL);
  
  // Reset sleeping
  resetSleep();
  
  // Initialize the main menu grid of buttons
  PHN_ButtonGrid menuGrid;
  menuGrid.setDimension(3, 3);
  menuGrid.setBounds(25, 30, 270, 204);
  menuGrid.setSpacing(15, 8);
  menuGrid.setColor(FRAME, BLACK);
  menuGrid.setColor(FOREGROUND, BUTTON_COLOR);
  menuGrid.setColor(HIGHLIGHT, RED);
  menuGrid.button(0).setImage(FLASH_MAPPED_Image(icon_call_data));
  menuGrid.button(1).setImage(FLASH_MAPPED_Image(icon_sendsms_data));
  menuGrid.button(2).setImage(FLASH_MAPPED_Image(icon_serial_data));
  menuGrid.button(3).setImage(FLASH_MAPPED_Image(icon_calllog_data));
  menuGrid.button(4).setImage(FLASH_MAPPED_Image(icon_messages_data));
  menuGrid.button(5).setImage(FLASH_MAPPED_Image(icon_contacts_data));
  menuGrid.button(6).setImage(FLASH_MAPPED_Image(icon_clock_data));
  menuGrid.button(7).setImage(FLASH_MAPPED_Image(icon_calculator_data));
  menuGrid.button(8).setImage(FLASH_MAPPED_Image(icon_sketches_data));

  // In main menu - show big header
  header.setBigHeader(true);
  header.refreshInfo();

  // Add the widgets, update until an option is chosen, then remove again
  display.addWidget(menuGrid);
  int clickedIdx;
  do {
    display.update();
    clickedIdx = menuGrid.getClickedIndex();

    // Handle sleeping
    updateSleep();

    // Handle incoming calls by showing the user a dialog with options
    boolean isCalled = sim.isCalled();
    boolean hasNewMessage = sim.hasNewMessage();
    if (isCalled || hasNewMessage) {
      // Hide menu
      menuGrid.setDrawingEnabled(false);
      menuGrid.setVisible(false);
      display.update();

      // Receive
      handleReceiving(isCalled, hasNewMessage);

      // Show grid again
      menuGrid.setVisible(true);
      menuGrid.setDrawingEnabled(true);
      header.setNavigation(NULL, NULL);
      display.invalidate();

      // Delay before sleeping again
      resetSleep();
    }
  } while (clickedIdx == -1);

  // Remove icons without drawing and revert header to big size
  header.setBigHeader(false);
  menuGrid.setDrawingEnabled(false);
  display.removeWidget(menuGrid);

  // Refresh/undo/update drawing
  display.update();

  // Return the result
  return clickedIdx;
}

void handleReceiving(boolean isCalled, boolean hasNewMessage) {

  // Draw frame of dialog box
  const int box_x = 25;
  const int box_y = 30;
  const int box_w = 270;
  const int box_h = 204;
  display.fillBorderRect(box_x, box_y, box_w, box_h, BLACK, WHITE);

  // Handle incoming call
  if (isCalled) {
    // Add decline button    
    PHN_Button declineBtn;
    declineBtn.setBounds(box_x+5, box_y+box_h-45, 100, 40);
    declineBtn.setColor(FOREGROUND, RED);
    declineBtn.setColor(FRAME, WHITE);
    declineBtn.setText("Decline");
    display.addWidget(declineBtn);
    
    // Add accept button
    PHN_Button acceptBtn;
    acceptBtn.setBounds(box_x+box_w-5-declineBtn.getWidth(), declineBtn.getY(), declineBtn.getWidth(), declineBtn.getHeight());
    acceptBtn.setColor(FOREGROUND, GREEN);
    acceptBtn.setColor(FRAME, WHITE);
    acceptBtn.setText("Accept");
    display.addWidget(acceptBtn);
    
    // Add call number label
    PHN_Label numberLabel;
    numberLabel.setBounds(box_x+5, box_y+5, box_w-10, 50);
    numberLabel.setText(sim.getIncomingNumber());
    display.addWidget(numberLabel);

    // Show a little more text
    display.setTextColor(YELLOW);
    display.drawStringMiddle(box_x+5, box_y+60, box_w-10, 90, "Is calling you...\n\nAccept the call?");

    // Update until option is chosen
    do {
      display.update();
    } while (!acceptBtn.isClicked() && !declineBtn.isClicked() && sim.isCalled());

    // Remove options again
    display.removeWidget(declineBtn);
    display.removeWidget(acceptBtn);
    display.removeWidget(numberLabel);

    // Handle the option chosen
    if (declineBtn.isClicked()) {
      // Reject and show menu again
      endCall();
      display.invalidate();
    } else if (acceptBtn.isClicked()) {
      // Accept in the startCall function called here
      header.setBigHeader(false);
      startCall();
      header.setBigHeader(true);
    } else {
      // Call ended otherwise
      display.invalidate();
    }
  }

  // Handle incoming SMS
  if (hasNewMessage) {
    SimMessage message = sim.getNewMessage();

    // Show information
    String toptxt;
    toptxt += "Message from ";
    if (message.sender.text[0]) {
      toptxt += message.sender.text;
    } else {
      toptxt += message.sender.number;
    }
    PHN_Label toptxtLabel;
    toptxtLabel.setBounds(box_x+5, box_y+5, box_w-10, 10);
    toptxtLabel.setText(toptxt);
    display.addWidget(toptxtLabel);

    // Add a textbox showing the contents
    PHN_TextBox messageTxt;
    messageTxt.setBounds(box_x+5, box_y+20, box_w-10, box_h-65);
    messageTxt.setTextSize(2);
    messageTxt.setMaxLength(strlen(message.text)+1);
    messageTxt.setText(message.text);
    display.addWidget(messageTxt);

    // Add decline button to close
    PHN_Button declineBtn;
    declineBtn.setBounds(box_x+5, box_y+box_h-35, 70, 30);
    declineBtn.setColor(FOREGROUND, RED);
    declineBtn.setColor(FRAME, WHITE);
    declineBtn.setText("Close");
    display.addWidget(declineBtn);

    // Add accept button to reply
    PHN_Button acceptBtn;
    acceptBtn.setBounds(box_x+box_w-5-declineBtn.getWidth(), declineBtn.getY(), declineBtn.getWidth(), declineBtn.getHeight());
    acceptBtn.setColor(FOREGROUND, GREEN);
    acceptBtn.setColor(FRAME, WHITE);
    acceptBtn.setText("Reply");
    display.addWidget(acceptBtn);

    // Wait until the user decides
    do {
      display.update();
    } while (!declineBtn.isClicked() && !acceptBtn.isClicked());
    
    // Remove widgets
    display.removeWidget(toptxtLabel);
    display.removeWidget(messageTxt);
    display.removeWidget(declineBtn);
    display.removeWidget(acceptBtn);

    // Reply if requested
    if (acceptBtn.isClicked()) {
      // Hide the main menu and quickly allow the user to reply
      // When done, add the main menu widgets again
      header.setBigHeader(false);
      sendMessage(message.sender.number);
      header.setBigHeader(true);
    }
  }
}

void handlePin() {
  // Check if the SIM PIN must be entered
  if (sim.isSimCardInserted() && (sim.getPinStatus() != SIM_PIN_STATUS_READY)) {

    // Header is used here
    PHN_PhoneHeader header;
    header.setNavigation(" Exit ", "Accept");
    display.addWidget(header);

    // Label showing current status
    PHN_Label codeStatus;
    codeStatus.setBounds(5, 15, 310, 30);
    display.addWidget(codeStatus);

    // Text box to show the PIN/PUC entered by the user
    PHN_TextBox codeText;
    codeText.setBounds(10, 40, 300, 32);
    codeText.setTextSize(3);
    codeText.setMaxLength(15);
    codeText.showBackspace(true);
    display.addWidget(codeText);

    // Keyboard with number layout for entering a PIN/PUC
    PHN_Keyboard codeKeys;
    codeKeys.setBounds(70, 80, 170, 132);
    codeKeys.setSpacing(4, 4);
    codeKeys.setDimension(3, 4);
    codeKeys.setKeys("123456789\r0\r");
    codeKeys.setColor(FOREGROUND, BUTTON_COLOR);
    display.addWidget(codeKeys);

    // Default, unentered PUK code
    char pukCode[20];
    pukCode[0] = 0;

    // Read in the code
    boolean updateStatus = true;
    boolean needPUC = false;
    boolean wasEntered = false;
    while (true) {
      // Update PIN Status
      if (updateStatus || wasEntered) {
        updateStatus = false;
        needPUC = false;

        int pinStatus = sim.getPinStatus();
        if (pinStatus == SIM_PIN_STATUS_READY) {
          break;
        }
        if (pinStatus == SIM_PIN_STATUS_NEEDPIN) {
          codeStatus.setText("Please enter SIM PIN");
        }
        if (pinStatus == SIM_PIN_STATUS_NEEDPUK) {
          codeStatus.setText("Please enter SIM unblocking PUK");
          needPUC = true;
        }
        if (wasEntered) {
          wasEntered = false;
          codeText.setText("Invalid code");
          display.update();
          delay(500);
        }
        codeText.setText("");
      }

      // Update the widgets
      display.update();

      // Enter text
      if (codeKeys.clickedKey()) {
        codeText.setSelection(codeKeys.clickedKey());
      }

      // Turn off SIM and go to sketch list when cancelling
      if (header.isCancelled()) {
        shutDown();
      }

      // Attempt to enter the current code when accepting
      if (header.isAccepted() && (codeText.textLength() > 0)) {
        if (needPUC) {
          if (pukCode[0] == 0) {
            strcpy(pukCode, codeText.text());
            codeText.setText("");
            codeStatus.setText("Please enter new pin code");
          } else {
            sim.enterPuk(pukCode, codeText.text());
            wasEntered = true;
          }
        } else {
          sim.enterPin(codeText.text());
          wasEntered = true;
        }
      }
    }

    // Clear all widgets previously added
    display.clearWidgets();
    display.update();
  }
}

void shutDown() {
  // Shut off SIM and go to Sketch List
  if (sim.isOn()) {
    display.fillBorderRect(10, 50, 300, 140, YELLOW, RED);
    display.setTextColor(RED, YELLOW);
    display.drawStringMiddle(10, 50, 300, 140, "Shutting down...");
    sim.end();
  }
  PHN_loadSketch("SKETCHES");
}

