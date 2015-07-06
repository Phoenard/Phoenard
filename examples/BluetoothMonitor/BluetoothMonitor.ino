/*
 * This is a Bluetooth communication demo for the Phoenard.
 * The sketch allows you to discover nearby Bluetooth devices,
 * connect to them or allow others to connect to you.
 * When connected, you can use an on-screen keyboard to send
 * and receive messages wirelessly.
 * 
 * The Bluetooth v4.0 module used is the HM-11, a datasheet
 * with the specs and AT-Commands can be found here:
 * http://txyz.info/b10n1c/datasheets/hm-11_bluetooth40_en.pdf
 *
 * The discovery part, where the device looks for devices it
 * can connect to and where connecting is initated, is located
 * in the 'BluetoothDiscovery' file of this sketch.
 *
 * The serial monitor part, where the user can communicate
 * with the connected device, is inside BluetoothSerial.
 */
#include "Phoenard.h"

// Serial BAUD rates that are selectable, by index as used by the module
const long baud_rates[] = {9600, 19200, 38400, 57600, 115200,
                           4800, 2400, 1200, 230400};

// Current baud rate index used
int currentBaud;

// Address of this module
char moduleAddress[21];

void setup() {
  Serial.begin(9600);
}

void loop() {
  showDiscovery();
}

/* Initializes the Bluetooth module into master mode */
void initializeBluetooth() {
  // Setup the Bluetooth registers for first use as a master (discovery mode)
  display.fill(BLACK);
  display.setCursor(3, 3);
  
  writeDisplayLog("Turning on module", WHITE);
  enableBluetooth();
  delay(500);
  writeDisplayLog("  OK\n", GREEN);
  
  // Figure out what baud rate the device runs at
  writeDisplayLog("Detecting used Baud rate", WHITE);
  currentBaud = 0;
  do {
    Serial2.begin(baud_rates[currentBaud]);
    if (sendCommand("AT")) {
      break;
    }
    
    currentBaud++;
  } while (currentBaud != 9);

  // Failed?
  if (currentBaud == 9) {
    writeDisplayLog("  ERROR\n", RED);
    for (;;);
  } else {
    writeDisplayLog("  OK\n", GREEN);
  }

  sendCommandLog("AT+RESET", "Resetting Bluetooth module");
  sendCommandLog("AT+ROLE1", "Setting Bluetooth role to master");
  sendCommandLog("AT+IMME1", "Setting Bluetooth notification mode");
  sendCommandLog("AT+SHOW1", "Setting Bluetooth name display");

  // Read module address: OK+ADDR:20C38FF42AA5
  writeDisplayLog("Reading device address", WHITE);
  if (sendCommand("AT+ADDR?") && Serial2.find("+ADDR:")) {
    int len = 0;
    while (len < 20 && waitAvailable(Serial2, 50)) {
      moduleAddress[len++] = Serial2.read();
    }
    moduleAddress[len] = 0;
    writeDisplayLog("  OK\n", GREEN);
  } else {
    writeDisplayLog("  ERROR\n", RED);
    for (;;);
  }

  writeDisplayLog("Waiting for device to initialize...", WHITE);
  delay(1000);
  flushRead(Serial2);
  display.fill(BLACK);
}

/* Sends an AT Command to the Bluetooth module, logging it and checking for errors */
void sendCommandLog(String command, String actionText) {
  // Show what is being done
  writeDisplayLog(actionText, WHITE);

  // Send command to the module
  if (sendCommand(command)) {
    // Ok!
    display.setTextColor(GREEN, BLACK);
    display.println("  OK");
  } else {
    // Notify error and halt the program
    display.setTextColor(RED, BLACK);
    display.println("  ERROR");
    for (;;);
  }
}

/* Sends an AT Command to the Bluetooth module, checking if an OK is received */
boolean sendCommand(String command) {
  Serial.println(command);
  Serial2.print(command);
  Serial2.flush();
  delay(5);
  return Serial2.find("OK");
}

/* Displays logging contents onto the screen */
void writeDisplayLog(String text, color_t color) {
  display.setTextSize(1);
  display.setTextColor(color, BLACK);
  display.print(text);
}
