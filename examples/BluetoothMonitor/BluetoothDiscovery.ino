/*
 * This part of the sketch shows a list of devices that can be connected with
 * It makes use of a buffer filled asynchronously as discovery data is updated
 */

#define CONN_LIST_SIZE 6  /* How many devices are shown at one time in the list */

/* A single discovered Bluetooth device entry */
typedef struct {
  boolean addressUpdated;
  boolean nameUpdated;
  char address[13];
  char name[20];
} BluetoothEntry;

/* All entries currently discovered and being discovered */
BluetoothEntry discovered[50];
int discovered_cnt = 0;

/* Buffer used to hold incoming data from the bluetooth module */
char searchBuffer[100];
int searchBufferLength;

/* Shows the discovery dialog */
void showDiscovery() {
  // Setup the module before starting discovery
  initializeBluetooth();

  // Setup connections list, initially empty
  PHN_ItemList connList;
  connList.setBounds(10, 34, 300, 160);
  connList.setPageSize(CONN_LIST_SIZE);
  connList.setItemCount(0);
  connList.setDrawFunction(bluetoothEntryDrawFunc);
  display.addWidget(connList);

  // Button to allow others to connect to me
  PHN_Button slaveBtn;
  slaveBtn.setBounds(10, 198, 145, 30);
  slaveBtn.setText("Connect to me\n(Slave Mode)");
  display.addWidget(slaveBtn);

  // Button to connect with the selected device
  PHN_Button connectBtn;
  connectBtn.setBounds(165, 198, 145, 30);
  connectBtn.setText("Connect as Master\n(Master Mode)");
  display.addWidget(connectBtn);

  // Label to show 'Baud:'
  PHN_Label baudLbl;
  baudLbl.setBounds(5, 5, 80, 22);
  baudLbl.setText("Baud rate:");
  display.addWidget(baudLbl);

  // Number dial to switch the baud rate used
  PHN_NumberBox baudBox;
  baudBox.setBounds(85, 5, 150, 22);
  baudBox.setRange(0, 8);
  baudBox.setValue(currentBaud);
  display.addWidget(baudBox);

  // Clear buffer
  setSearchBufferLength(0);

  long searchTimeout;
  boolean isSearchRunning = false;
  boolean isMasterMode = true;
  while (true) {
    // Update the widgets
    display.update();

    // Update baud
    if (baudBox.isValueChanged()) {
      // Update displayed label
      currentBaud = baudBox.value();
      baudBox.setText(baud_rates[currentBaud]);

      // Update baud setting of module
      // Note: changes take effect after connecting
      // We should not change baud rate of the Serial
      String command = "AT+BAUD";
      command += currentBaud;
      if (!sendCommand(command)) {
        showSearchStatus("Failed to switch baud rates");
      }
    }

    // Switch between master/slave mode
    boolean startSlave = slaveBtn.isClicked();
    boolean startMaster = (connectBtn.isClicked() && connList.itemCount() > 0);
    if (startSlave || startMaster) {
      // Wait for any discovery to finish
      if (isSearchRunning) {
        isSearchRunning = false;
        showSearchStatus("Terminating device discovery...");

        // Wait until 'DISCE' is read from the device
        char buff[5];
        memset(buff, 0, 5);
        while (waitAvailable(Serial2, 5000)) {
          memmove(buff, buff+1, 4);
          buff[4] = Serial2.read();

          // Check if found
          if (memcmp(buff, "DISCE", 5) == 0) {
            break;
          }
        }
      }

      if (startSlave) {
        showSearchStatus("Switching mode...");
        if (sendCommand("AT+ROLE0")) {
          // Clear all widgets and enter serial mode
          // Do not continue after this to re-start discovery later
          display.clearWidgets();
          showSearchStatus("");
          showSerial();
          return;
        } else {
          showSearchStatus("Failed to switch to slave mode");
        }
      }
      if (startMaster) {
        showSearchStatus("Connecting...");
        String commandStr = "AT+CONN";
        commandStr += (connList.selectedIndex());
        if (sendCommand(commandStr)) {
          // Clear all widgets and enter serial mode
          // Do not continue after this to re-start discovery later
          display.clearWidgets();
          showSearchStatus("");
          showSerial();
          return;
        } else {
          showSearchStatus("Failed to connect");
        }
      }
    }

    /*
     * Device discovery logic down below
     * Message exchange format:
     *   Send:
     *     AT+DISC?
     *   Response:
     *     OK+DISCS
     *     OK+DIS0:B4994C71121E
     *     OK+NAME:HMSoft
     *     OK+DISCE
     */

    // Start searching if searching ended or timed out
    if (isMasterMode && (!isSearchRunning || ((millis() - searchTimeout) >= 10000))) {
      isSearchRunning = true;
      searchTimeout = millis();
      Serial2.print("AT+DISC?");
      
      // If no items are displayed yet; show we are searching
      if (connList.itemCount() == 0) {
        display.setTextColor(GREEN, BLACK);
        display.drawStringMiddle(20, 80, 280, 60,  "Searching...");
      }
    }

    // Whether a new response is ready to be read
    boolean hasResponse = false;

    // Fill input buffer, filtering out newline characters
    if (Serial2.available()) {
      char c = Serial2.read();
      if (c != '\n' && c != '\r') {
        searchBuffer[searchBufferLength] = c;
        setSearchBufferLength(searchBufferLength + 1);
      } else {
        hasResponse = true;
      }

      // Received things; reset timeout
      searchTimeout = millis();
    }

    // Check if start token is read
    if (searchBufferLength >= 3 && strcmp(searchBuffer+searchBufferLength-3, "OK+") == 0) {
      setSearchBufferLength(searchBufferLength-3);
      hasResponse = true;
    }
     
    // Read in the response
    if (hasResponse && searchBufferLength > 0) {
      // Reference to entry currently being read
      BluetoothEntry &lastEntry = discovered[discovered_cnt];

      // Handle possible responses:
      if (strcmp(searchBuffer, "DISCS") == 0) {
        // DISCS = discovery start; reset updated state for next run
        for (int i = 0; i < discovered_cnt; i++) {
          discovered[i].nameUpdated = false;
          discovered[i].addressUpdated = false;
        }
        discovered_cnt = 0;
    
      } else if (strncmp(searchBuffer, "DISCE", 5) == 0) {
        // DISCE discovery end; restart discovery and refresh list
        connList.setItemCount(discovered_cnt);
        connList.invalidate();
        isSearchRunning = false;
    
      } else if (strncmp(searchBuffer, "DIS", 3) == 0) {
        // DIS0: new discovered device address
        strncpy(lastEntry.address, searchBuffer+5, sizeof(lastEntry.address)-1);
        lastEntry.addressUpdated = true;

      } else if (strncmp(searchBuffer, "NAME:", 5) == 0) {
        // NAME: name for previously discovered device address
        strncpy(lastEntry.name, searchBuffer+5, sizeof(lastEntry.name)-1);
        lastEntry.nameUpdated = true;
      }

      // Go to next entry when completed
      if (lastEntry.nameUpdated && lastEntry.addressUpdated) {
        discovered_cnt++;
        if (discovered_cnt > connList.itemCount()) {
          connList.setItemCount(discovered_cnt);
        }
      }

      // Reset buffer length
      setSearchBufferLength(0);
    }
  }
}

/* Shows status info in the discovery screen */
void showSearchStatus(String text) {
  const int status_y = 231;
  display.fillRect(10, status_y, 300, 8, BLACK);
  display.setTextColor(RED, BLACK);
  display.setTextSize(1);
  display.setCursor(10, status_y);
  display.print(text);
}

/* Sets the search buffer to a certain length */
void setSearchBufferLength(int length) {
  searchBufferLength = length;
  searchBuffer[length] = 0;
}

/* Function to draw Bluetooth Entries in an Item List */
void bluetoothEntryDrawFunc(ItemParam &p) {
  BluetoothEntry &entry = discovered[p.index];

  // Fill background color
  display.fillRect(p.x, p.y, p.w, p.h, p.color);

  // Draw address on the right in red
  display.setTextColor(RED, p.color);
  display.drawStringMiddle(p.x+p.w-80, p.y, 80, p.h, entry.address);

  // Draw name on the left
  display.setTextColor(BLACK, p.color);
  display.setTextSize(2);
  display.setCursor(p.x+3, p.y+3);
  display.print(entry.name);
}
