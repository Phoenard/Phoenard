/*
 * This part of the sketch deals with composing, sending and reading text messages
 */

/* Amount of contacts showed at one time (page size) */
const int MESSAGES_PAGE_SIZE = 7;

/* Shows a list of inbox messages stored on the SIM */
void showMessages() {
  // Initialize SRAM as needed
  sram.begin();
  
  PHN_ItemList list;
  list.setBounds(10, 20, 300, 170);
  list.setItemCount(sim.getMessageCount());
  list.setDrawFunction(messageDrawFunc);
  list.setPageSize(MESSAGES_PAGE_SIZE);
  list.setScrollWidth(20);
  display.addWidget(list);

  boolean reloadMessages = true;
  boolean enableList = true;
  int readIndex = 0;
  int readSimIndex = 0;
  int messageLimit = sim.getMessageLimit();
  do {
    if (enableList) {
      enableList = false;
      header.setNavigation(" Back ", " Read ");
      list.setVisible(true);
    }
    if (reloadMessages) {
      reloadMessages = false;
      readIndex = 0;
      readSimIndex = 0;
      list.setItemCount(sim.getMessageCount());

      // Set the 'valid' byte field to 0 for all entries displayed
      for (int i = 0; i < list.itemCount(); i++) {
        sram.write(i * sizeof(SimMessage), 0);
      }
    }

    // Refresh widgets
    display.update();

    // Read messages every loop
    if ((readIndex < list.itemCount()) && (readSimIndex < messageLimit)) {
      SimMessage nextMessage = sim.getMessage(readSimIndex++);
      if (nextMessage.valid) {
        sram.writeSegment(readIndex, &nextMessage, sizeof(SimMessage));
        list.drawItem(readIndex);
        readIndex++;
      }
    }

    // If accepted, show the message
    if (header.isAccepted()) {
      SimMessage message;
      sram.readSegment(list.selectedIndex(), &message, sizeof(SimMessage));
      if (message.valid) {
        list.setVisible(false);

        // Show message, reload if message was changed / deleted
        reloadMessages = showMessage(message);

        // Enable the list to be shown again
        enableList = true;
        display.update();
      }
    }
  } while (!header.isCancelled());

  display.removeWidget(list);
}

/* Shows the contents of a text message */
boolean showMessage(SimMessage &message) {
  header.setNavigation(" Back ", NULL);

  // Textbox in which message is shown
  PHN_TextBox messageText;
  messageText.setBounds(10, 35, 300, 155);
  messageText.showCursor(false);
  messageText.setMaxLength(strlen(message.text));
  messageText.setText(message.text);
  messageText.showScrollbar(true);
  display.addWidget(messageText);

  // Button to delete the message
  PHN_Button deleteBtn;
  deleteBtn.setBounds(70, 203, 55, 32);
  deleteBtn.setText("Delete");
  deleteBtn.setColor(FOREGROUND, RED);
  deleteBtn.setColor(FRAME, RED);
  deleteBtn.setColor(CONTENT, BLACK);
  display.addWidget(deleteBtn);

  // Button to call the sender
  PHN_Button callBtn;
  callBtn.setBounds(195, 203, 55, 32);
  callBtn.setText(" Call ");
  callBtn.setColor(FOREGROUND, GREEN);
  callBtn.setColor(FRAME, GREEN);
  callBtn.setColor(CONTENT, BLACK);
  display.addWidget(callBtn);

  // Button to send the sender a text message reply
  PHN_Button replyBtn;
  replyBtn.setBounds(260, 203, 55, 32);
  replyBtn.setText("Reply");
  replyBtn.setColor(FOREGROUND, GREEN);
  replyBtn.setColor(FRAME, GREEN);
  replyBtn.setColor(CONTENT, BLACK);
  display.addWidget(replyBtn);

  // Update to make sure screen is synchronized
  display.update();

  // Display sender and date information at the top
  display.setTextColor(WHITE, BLACK);
  display.setTextSize(1);
  display.setCursor(10, 18);
  display.print(message.sender.text);
  display.print('(');
  display.print(message.sender.number);
  display.print(")  ");
  display.printDate(message.date);
  display.print(' ');
  display.printTime(message.date);

  boolean call = false;
  boolean reply = false;
  do {
    display.update();
    call = callBtn.isClicked();
    reply = replyBtn.isClicked();
  } while (!header.isCancelled() && !deleteBtn.isClicked() && !call && !reply);

  display.removeWidget(messageText);
  display.removeWidget(deleteBtn);
  display.removeWidget(callBtn);
  display.removeWidget(replyBtn);
  
  // Wipe top area where we displayed the number
  display.fillRect(10, 18, 300, 8, BLACK);
  
  if (deleteBtn.isClicked()) {
    sim.deleteMessage(message.index);
    return true;
  }

  if (call) {
    startCall(message.sender.number);
  } else if (reply) {
    sendMessage(message.sender.number);
  }

  return false;
}

/* Allows the user to enter a phone number and then compose a text message to send out */
void sendMessage() {
  // First ask for a number to send the message to
  char number[20];
  if (enterNumber(number)) {
    sendMessage(number);
  }
}

/* Allows the user to compose a text message to send out */
void sendMessage(const char* number) {
  // Update navigation
  header.setNavigation("Cancel", " Send ");

  // Add textbox for displaying the message being typed
  PHN_TextBox text;
  text.setBounds(7, 15, 307, 86);
  text.setTextSize(2);
  text.showScrollbar(true);
  display.addWidget(text);
  
  // Add keyboard for entering text
  PHN_Keyboard keyboard;
  keyboard.setBounds(7, 104, 310, 113);
  keyboard.setDimension(11, 5);
  keyboard.setSpacing(2);
  keyboard.addKeys("abc", "1234567890."
                          "qwertyuiop,"
                          "asdfghjkl\b\b"
                          "\f\fzxcvbnm\n\n"
                          "\r\r\r     \r\r\r");
  keyboard.addKeys("ABC", "1234567890."
                          "QWERTYUIOP,"
                          "ASDFGHJKL\b\b"
                          "\f\fZXCVBNM\n\n"
                          "\r\r\r     \r\r\r");
  keyboard.addKeys("?!.", "!@#$%^&*()."
                          "-+_=\\/?~<>,"
                          ":;\'\"[]{}|\b\b"
                          "\f\f!?.,/\\\"\n\n"
                          "\r\r\r     \r\r\r");

  display.addWidget(keyboard);

  // Show dummy number text until screen is touched
  String dummyText = "Message for ";
  dummyText += number;
  text.setText(dummyText);
  text.setColor(CONTENT, GRAY_LIGHT);
  text.setTextSize(1);
  text.showCursor(false);
  while (!display.isTouchDown()) {
    display.update();
  }
  text.setText("");
  text.setColor(CONTENT, BLACK);
  text.setTextSize(2);
  text.showCursor(true);

  while (true) {
    // Update the log
    display.update();

    // Handle keyboard input
    if (keyboard.clickedKey()) {
      text.setSelection(keyboard.clickedKey());
    }

    // Try to send the message
    if (header.isAccepted()) {
      if (sim.sendMessage(number, text.text())) {
        header.showStatus("Message sent.");
        break;
      } else {
        header.showStatus("Failed to send message");
        keyboard.invalidate();
      }
    }

    // Cancelled
    if (header.isCancelled()) {
      break;
    }
  }

  // Remove widgets and finish
  display.removeWidget(text);
  display.removeWidget(keyboard);
}

void messageDrawFunc(ItemParam &p) {
  SimMessage message;
  sram.readSegment(p.index, &message, sizeof(SimMessage));

  display.fillRect(p.x, p.y, p.w, p.h, p.color);
  display.setTextColor(BLACK, p.color);
  if (message.valid) {
    // Loaded, show message preview
    const char* text = message.text;
    const char* sender;
    if (message.sender.text[0]) {
      sender = message.sender.text;
    } else {
      sender = message.sender.number;
    }

    // First print the date
    display.setTextSize(1);
    display.setCursor(p.x+3, p.y+3);
    display.printDate(message.date);
    display.print(' ');
    display.printShortTime(message.date);
    
    // Then draw the name or address of the sender below it
    display.setCursor(p.x+3, p.y+13);
    display.setTextColor(BLUE, p.color);
    for (int i = 0; i < 14 && sender[i]; i++) {
      display.print(sender[i]);
    }
    
    // Draw a fancy border line
    display.drawVerticalLine(p.x + 94, p.y, p.h, RED);
    
    // Draw part of the message next to this
    display.setCursor(p.x+97, p.y+3);
    display.setTextColor(GRAY, p.color);
    const int line_len = 30;
    for (int i = 0; i < (line_len*2) && text[i]; i++) {
      if (i == line_len) {
        display.setCursor(p.x+97, p.y+13);
      }
      if (text[i] == '\r') continue;
      if (text[i] == '\n') {
        display.print(' ');
      } else {
        display.print(text[i]);
      }
    }
  } else {
    // Not yet loaded, show ...
    display.drawStringMiddle(p.x, p.y, p.w, p.h, "...");
  }
}
