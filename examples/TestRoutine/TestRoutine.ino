/*
 * This is a full-fledged test routine used to validate Phoenard operation.
 * All connections to on-board components are tested where possible.
 * For sensors, an additional damage check is performed.
 * This means sample data is measured and the range/deviation is checked.
 * Heavily fluctuating or extreme outputs indicate hardware failure.
 *
 * For testing the connector, the Phoenard must be tested in our test station.
 * This test station also performs the flashing of the bootloader/uploading.
 * On startup a check is performed whether the test station is active.
 *
 * All tests are located in the tests.h file.
 */
#include "Phoenard.h"
#include <Wire.h>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include <SPI.h>
#include <SD.h>
#include <Adafruit_VS1053.h>
#include <SFE_BMP180.h>
#include "Tests.h"

int testIndex = 0;

TestResult doTest(char* what, TestResult(*testFunc)(void)) {
  // Check if pressed - if pressed for longer than 1 second, show message
  long sel_start = millis();
  while(isSelectPressed() && (millis() - sel_start) < 1000);

  // Wait until the SELECT key is no longer pressed
  if (isSelectPressed()) {
    showMessage("Please release the SELECT button\n"
                "Indicates hardware problem if not pressed");

    while (isSelectPressed());
    showMessage("");
  }
  
  Serial.print(what);
  Serial.print(" Testing...");
  
  // Show testing state
  showMessage("");
  showStatus(testIndex, YELLOW, what, "Testing...");

  // Do the test
  TestResult result = testFunc();

  // Show result
  showStatus(testIndex, result.success ? GREEN : RED, what, result.status);
  Serial.print(result.success ? " SUCCESS" : " FAILURE");
  Serial.print(" - ");
  Serial.println(result.status);

  // Next test
  testIndex++;
  
  return result;
}

void showStatus(int index, color_t color, char* what, char* text) {
  const int w = 310;
  const int h = 13;
  int x = 5;
  int y = 5 + index * (h + 5);
  display.fillRect(x, y, w, h, color);
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(x + 3, y + 3);
  display.print(what);
  display.print(": ");
  display.print(text);
}

void setup() {
  Serial.begin(57600);

  delay(20);

  // Send out a token indicating we request a test station
  Serial.println("Ready");

  // Wait for a very short time for a response back to initiate test station mode
  isStationConnected = readToken(Serial, "Station OK", 200);

  // Setup eeprom to load home menu without saving on next reset
  if (isStationConnected) {
    PHN_Settings settings;
    PHN_Settings_Load(settings);
    memcpy(settings.sketch_toload, "SKETCHES", 8);
    settings.flags = SETTINGS_LOAD;
    PHN_Settings_Save(settings);
  }

  // LCD Test
  startLCDTest();

  // Indicate testing has started
  Serial.println("Testing started.");

  // Turn on SIM module as needed
  // Do this in two steps to save time
  sim.init();
  long sim_on_time = millis();
  if (sim.isOn()) {
    sim_on_time -= SIM_PWR_DELAY;
  } else {
    digitalWrite(SIM_PWRKEY_PIN, HIGH);
  }

  doTest("Connector", testConnector);
  doTest("Touchscreen", testTouchscreen);
  doTest("MIDI", testMIDI);
  doTest("MP3", testMP3);
  doTest("BMP180", testBMP180);
  doTest("MPU6050", testMPU6050);
  doTest("HMC5883L", testHMC5883L);
  doTest("SRAM", testRAM);
  doTest("Micro-SD", testSD);
  doTest("WiFi", testWiFi);
  doTest("Bluetooth", testBluetooth);

  // Wait until delay has elapsed
  while ((millis() - sim_on_time) < SIM_PWR_DELAY);
  digitalWrite(SIM_PWRKEY_PIN, LOW);

  // Perform testing
  doTest("SIM908", testSIM);

  sim.end();

  // All done!
  Serial.println("Testing completed.");
}

void loop() {
}

void startLCDTest() {
  // Show the LCD Version in Serial for debugging purposes
  Serial.print("LCD Version ID: ");
  Serial.println(PHNDisplayHW::readRegister(0), HEX);
  Serial.println("Press SELECT to continue...");

  // As a first test, show RGB colors on the screen to verify readout works as expected
  display.fillRect(0, 0, 107, 240, RED);
  display.fillRect(107, 0, 106, 240, GREEN);
  display.fillRect(213, 0, 107, 240, BLUE);
  display.setCursor(30, 30);
  display.setTextColor(WHITE);
  display.setTextSize(3);
  display.print("LCD Test Screen");
  display.setTextSize(2);
  display.setCursor(70, 100);
  display.print("Press SELECT to");
  display.setCursor(70, 120);
  display.print("start the test");
  
  // Wait for SELECT pressed
  while (!isSelectPressed());
  
  // Wipe screen
  display.fill(BLACK);
}
