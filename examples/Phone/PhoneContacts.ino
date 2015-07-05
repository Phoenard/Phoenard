/* 
 * This part of the sketch deals with showing a list of contacts and individual contact information
 * Since the SIM makes use of 'phone books' (obtained using AT+CPBS=?) this part is a little tricky.
 * For convenience, here is a refsheet of all the phone books and what they (should) contain:
 *
 * SIM900 AT Command Manual v1.03 - Page 70:
 * - "DC" ME dialed calls list(+CPBW may not be applicable
 *        for this storage)(same as LD)
 * - "EN" SIM (or MT) emergency number (+CPBW is not
 *        be applicable for this storage)
 * - "FD" SIM fix dialing-phone book. If a SIM card is
 *        present or if a UICC with an active GSM
 *        application is present, the information in EFFDN
 *        under DFTelecom is selected
 * - "MC" MT missed (unanswered received) calls list
 *        (+CPBW may not be applicable for this storage)
 * - "ON" SIM (or MT) own numbers (MSISDNs) list
 *        (reading of this storage may be available through
 *        +CNUM also). When storing information in the
 *        SIM/UICC, if a SIM card is present or if a UICC
 *        with an active GSM application is present, the
 *        information in EFMSISDN under DFTelecom is
 *        selected.
 * - "RC" MT received calls list (+CPBW may not be
 *        applicable for this storage)
 * - "SM" SIM/UICC phonebook. If a SIM card is present or
 *        if a UICC with an active GSM application is present, the
 *        EFADN under DFTelecom is selected.
 * - "LA" Last Number All list (LND/LNM/LNR)
 * - "ME" ME phonebook
 * - "BN" SIM barred dialed number
 * - "SD" SIM service dial number
 * - "VM" SIM voice mailbox
 * - "LD" SIM last-dialing-phone book
 */

/* Amount of contacts showed at one time (page size) */
const int CONTACT_PAGE_SIZE = 10;

void showContacts(const char* phoneBook, boolean allowEditing) {
  // Initialize sram as needed
  sram.begin();
  
  // Set SIM to use the phone book specified
  sim.setContactBook(phoneBook);

  // Button to add new contacts
  PHN_Button addBtn;
  addBtn.setBounds(190, 203, 55, 32);
  addBtn.setText(" Add ");
  addBtn.setColor(FOREGROUND, GREEN);
  addBtn.setColor(FRAME, GREEN);
  addBtn.setColor(CONTENT, BLACK);
  addBtn.setVisible(allowEditing);
  display.addWidget(addBtn);

  // Button to switch the book shown
  PHN_Button bookBtn;
  bookBtn.setBounds(130, 203, 55, 32);

  // List widget that shows a scrollable list of items
  PHN_ItemList list;
  list.setBounds(10, 18, 300, 182);
  list.setPageSize(CONTACT_PAGE_SIZE);
  list.setDrawFunction(contactDrawFunc);
  display.addWidget(list);

  boolean reloadContacts = true;
  boolean enableList = true;
  int readIndex = 0;
  int readSimIndex = 0;
  int contactLimit = sim.getContactLimit();
  do {
    if (enableList) {
      enableList = false;
      header.setNavigation(" Back ", "Options");
      list.setVisible(true);
      addBtn.setVisible(true);
    }
    if (reloadContacts) {
      reloadContacts = false;
      readIndex = 0;
      readSimIndex = 0;
      list.setItemCount(sim.getContactCount());

      // Set the 'valid' byte field to 0 for all entries displayed
      for (int i = 0; i < list.itemCount(); i++) {
        sram.write(i * sizeof(SimContact), 0);
      }
    }

    // Refresh widgets
    display.update();

    // Read contacts every loop
    if (readIndex < list.itemCount()) {
      if (readSimIndex < contactLimit) {
        SimContact nextContact = sim.getContact(readSimIndex++);
        if (nextContact.valid) {
          sram.writeSegment(readIndex, &nextContact, sizeof(SimContact));
          list.drawItem(readIndex);
          readIndex++;
        }
      } else {
        // Too many entries predicted; remove entries
        list.setItemCount(readIndex);
      }
    }

    // Show options menu
    int selIndex = list.selectedIndex();
    boolean addClicked = addBtn.isClicked();
    if (addClicked) {
      // Hide listing
      list.setVisible(false);
      addBtn.setVisible(false);
      header.setNavigation(" Back ", NULL);
      display.update();
        
      // Show add contact dialog
      SimContact newContact;
      newContact.text[0] = 0;
      newContact.number[0] = 0;
      if (editContact(newContact)) {
        sim.addContact(newContact);
        reloadContacts = true;
      }

      // Show list again
      enableList = true;
      display.update();
    }

    if (header.isAccepted()) {
      SimContact contact;
      sram.readSegment(selIndex, &contact, sizeof(SimContact));
      if (contact.valid) {     
        // Hide listing
        list.setVisible(false);
        addBtn.setVisible(false);
        header.setNavigation(" Back ", NULL);
        display.update();

        // Show options; reload if dialog changed things
        reloadContacts = showContactOptions(contact, allowEditing);
        
        // Show list again
        enableList = true;
        display.update();
      }
    }
  } while (!header.isCancelled());
  
  display.removeWidget(list);
  display.removeWidget(addBtn);
}

/* Shows options for a particular contact */
boolean showContactOptions(SimContact contact, boolean allowEditing) {
  // Add relevant widgets
  PHN_Label textLbl;
  textLbl.setBounds(10, 20, 300, 30);
  textLbl.setText(contact.text);
  display.addWidget(textLbl);
  
  PHN_Button options[4];
  options[0].setText("Start a call");
  options[1].setText("Send a message");
  options[2].setText("Edit contact");
  options[3].setText("Delete contact");
  for (int i = 0; i < 4; i++) {
    options[i].setBounds(65, 57 + 35 * i, 200, 30);
    display.addWidget(options[i]);
  }
  
  // These buttons only visible if editing
  options[2].setVisible(allowEditing);
  options[3].setVisible(allowEditing);
  
  // Show options
  int optionClicked = -1;
  do {
    display.update();
    for (int i = 0; i < 4; i++) {
      if (options[i].isClicked()) {
        optionClicked = i;
        break;
      }
    }
  } while ((optionClicked == -1) && !header.isCancelled());
  
  // Remove widgets again
  display.removeWidget(textLbl);
  for (int i = 0; i < 4; i++) {
    display.removeWidget(options[i]);
  }
  
  // Handle the option chosen
  if (optionClicked == 0) {
    // Start a voice call
    startCall(contact.number);
  } else if (optionClicked == 1) {
    // Send a text message
    sendMessage(contact.number);
  } else if (optionClicked == 2) {
    // Edit contact info
    if (editContact(contact)) {
      sim.setContact(contact.index, contact);
    }
    return true; // Changes happened
  } else if (optionClicked == 3) {
    // Delete the contact
    sim.deleteContact(contact.index);
    return true; // Changes happened
  }
  return false;
}

/* Allows the user to edit or fill in the contents of a SIM contact */
boolean editContact(SimContact &contact) {
  header.setNavigation("Cancel", " Done ");

  // Track the text field that is clicked
  boolean textClicked = true;
  boolean numberClicked = false;

  PHN_Label textLbl;
  textLbl.setBounds(10, 30, 50, 24);
  textLbl.setText("  Name:");
  display.addWidget(textLbl);

  PHN_TextBox textBox;
  textBox.setBounds(60, 30, 255, 24);
  textBox.setTextSize(2);
  textBox.setMaxLength(sim.getContactTextLimit());
  textBox.setText(contact.text);
  textBox.showBackspace(true);
  display.addWidget(textBox);

  PHN_Label numberLbl;
  numberLbl.setBounds(10, 63, 50, 24);
  numberLbl.setText("Number:");
  display.addWidget(numberLbl);

  PHN_TextBox numberBox;
  numberBox.setBounds(60, 63, 255, 24);
  numberBox.setTextSize(2);
  numberBox.setMaxLength(sim.getContactNumberLimit());
  numberBox.setText(contact.number);
  numberBox.showBackspace(true);
  display.addWidget(numberBox);

  // Add keyboard for entering text
  PHN_Keyboard keyboard;
  keyboard.setBounds(7, 102, 310, 113);
  keyboard.setDimension(10, 5);
  keyboard.setSpacing(2);
  keyboard.addKeys("abc", "1234567890"
                          "qwertyuiop"
                          "asdfghjkl."
                          "\f\fzxcvbnm,"
                          "\r\r\r    \r\r\r");
  keyboard.addKeys("ABC", "1234567890"
                          "QWERTYUIOP"
                          "ASDFGHJKL."
                          "\f\fZXCVBNM,"
                          "\r\r\r    \r\r\r");
  keyboard.addKeys("?!.", "!@#$%^&*()"
                          "-+_=\\/?~<>"
                          ":;\'\"[]{}|."
                          "\f\f!?.,/\\\","
                          "\r\r\r    \r\r\r");
  display.addWidget(keyboard);

  boolean boxChanged = true;
  do {
    if (boxChanged) {
      boxChanged = false;
      textBox.showCursor(textClicked);
      textBox.setColor(FOREGROUND, textClicked ? WHITE : GRAY_LIGHT);
      numberBox.showCursor(numberClicked);
      numberBox.setColor(FOREGROUND, numberClicked ? WHITE : GRAY_LIGHT);
    }
    
    display.update();
    
    if (!textClicked && textBox.isTouched()) {
      textClicked = true;
      numberClicked = false;
      boxChanged = true;
    }
    if (!numberClicked && numberBox.isTouched()) {
      textClicked = false;
      numberClicked = true;
      boxChanged = true;
    }
    if (keyboard.clickedKey()) {
      if (textClicked) textBox.setSelection(keyboard.clickedKey());
      if (numberClicked) numberBox.setSelection(keyboard.clickedKey());
    }
  } while (!header.isNavigating());
  
  display.removeWidget(textBox);
  display.removeWidget(numberBox);
  display.removeWidget(textLbl);
  display.removeWidget(numberLbl);
  display.removeWidget(keyboard);
  
  if (header.isAccepted()) {
    strcpy(contact.text, textBox.text());
    strcpy(contact.number, numberBox.text());
    return true;
  }
  return false;
}

void contactDrawFunc(ItemParam &p) {
  SimContact contact;
  sram.readSegment(p.index, &contact, sizeof(SimContact));
  const char* text;
  const char* number;
  if (contact.valid) {
    text = contact.text;
    number = contact.number;
  } else {
    text = "...";
    number = "...";
  }
  int w_text = p.w*4/7;
  int w_num = p.w - w_text;
  display.fillRect(p.x, p.y, p.w, p.h, p.color);
  display.setTextColor(BLACK, p.color);
  display.drawStringMiddle(p.x, p.y, w_text, p.h, text);
  display.drawStringMiddle(p.x+w_text, p.y, w_num, p.h, number);
}
