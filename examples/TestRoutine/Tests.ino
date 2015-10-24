#include "Phoenard.h"
#include <Wire.h>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include "HMC5883L.h"
#include <SPI.h>
#include <SD.h>
#include <Adafruit_VS1053.h>
#include <SFE_BMP180.h>
#include "TestResult.h"

/* All the BAUD rates to probe at */
long baud_rates[] = {9600, 115200, 19200, 38400, 57600, 4800, 2400, 1200, 230400};
const int baud_rates_cnt = sizeof(baud_rates) / sizeof(int);

void calc_stats(int16_t *values, int cnt, int32_t *mean, int32_t *variation) {
  int32_t diff;
  *mean = 0;
  for (int i = 0; i < cnt; i++) {
    *mean += values[i];
  }
  *mean /= cnt;
  *variation = 0;
  for (int i = 0; i < cnt; i++) {
    diff = values[i] - *mean;
    *variation += (diff * diff);
  }
  *variation /= cnt;
  *variation = (int32_t) sqrt(*variation);
}

void print_sensor_info(int32_t *mean, int32_t *deviation, int count) {
  Serial.print("mean=[");
  for (int i = 0; i < count; i++) {
    if (i) Serial.print(", ");
    Serial.print(mean[i]);
  }
  Serial.print("]  deviation=[");
  for (int i = 0; i < count; i++) {
    if (i) Serial.print(", ");
    Serial.print(deviation[i]);
  }
  Serial.println("]");
}

TestResult SUCCESS_RESULT(true, "No problems found");
TestResult NOCONN_RESULT(false, "No connection with device");

boolean readToken(Stream &serial, const char *token, unsigned long timeoutMS) {
  uint16_t i = 0;
  unsigned long startTime = millis();
  while (token[i]) {
    if ((millis() - startTime) >= timeoutMS) {
      // Timeout
      return false;
    }
    if (serial.available() && serial.read() != token[i++]) i = 0;
  }
  return true;
}

boolean writeATCommand(Stream &serial, const char *command, const char* response, long timeoutMS) {
  for (int i = 0; i < 3; i++) {
    serial.print(command);
    if (readToken(serial, response, timeoutMS)) return true;
  }
  return false;
}

void set_BlueWifi_baud(long baud) {
  Serial.println();
  Serial.print(F("  SETTING BAUD="));
  Serial.print(baud);
  Serial.print(F(" | "));
  Serial2.begin(baud);
}

bool SpiRAMTest(uint16_t address, char data_byte) {
  sram.write(address, data_byte);
  char result = sram.read(address);
  if (result == data_byte) return true;

  Serial.print(F("Data Error at address ["));
  Serial.print(address);
  Serial.print(F("]: "));
  Serial.print(result);
  Serial.print(F(" != "));
  Serial.print((uint8_t) data_byte);
  Serial.print(' ');
  return false;
}

// Shows a counter on the LCD screen
void showCounter(int ctr) {
  display.setTextColor(YELLOW, BLACK);
  display.setCursor(320-20, 240-15);
  display.print(ctr);
}

// Show a message on the LCD screen
boolean has_message = false;
void showMessage(const char* message) {
  if (!has_message && !*message) {
    return;
  }
  has_message = *message;
  
  // Wipe the previous contents and draw the text
  const int msg_w = 320;
  const int msg_h = 30;
  const int msg_x = 0;
  const int msg_y = 240 - msg_h;
  display.fillRect(msg_x, msg_y, msg_w, msg_h, BLACK);
  display.setTextColor(YELLOW);
  display.drawStringMiddle(msg_x, msg_y, msg_w, msg_h, message);
  
  // Also print to SERIAL
  if (has_message) {
    Serial.println();
    Serial.print("  [");
    while (*message) {
      if (*message == '\n') {
        Serial.print(' ');
      } else {
        Serial.print(*message);
      }
      message++;
    }
    Serial.println("]");
  }
}

boolean doPinPullHighTest(int pin) {
  pinMode(pin, INPUT);
  digitalWrite(pin, HIGH);
  delay(50);
  return (digitalRead(pin) == LOW);
}

boolean isScreenTouched() {
  uint16_t x, y;
  float pressure;
  PHNDisplayHW::readTouch(&x, &y, &pressure);
  return pressure >= PHNDisplayHW::PRESSURE_THRESHOLD;
}

/** ========================================================== **/

TestResult testScreen() {
  // Put LCD YP/XM pins output LOW
  // The pins with pullup HIGH YM and XP should be LOW as well, otherwise is disconnected
  // This does a basic connection test of several LCD pins
  digitalWrite(TFTLCD_YP_PIN, LOW);
  digitalWrite(TFTLCD_XM_PIN, LOW);
  if (!doPinPullHighTest(TFTLCD_YM_PIN)) {
    return TestResult(false, F("Disconnected pins: Data[7] - WR"));
  }
  if (!doPinPullHighTest(TFTLCD_XM_PIN)) {
    return TestResult(false, F("Disconnected pins: Data[6] - RS"));
  }
  digitalWrite(TFTLCD_YP_PIN, HIGH);
  digitalWrite(TFTLCD_XM_PIN, HIGH);
  pinMode(TFTLCD_YM_PIN, OUTPUT);
  pinMode(TFTLCD_XM_PIN, OUTPUT);
  
  // First try to read the LCD version ID
  // This provides a basic check for the RS/WR/data pins
  uint16_t lcd_version = PHNDisplayHW::readRegister(0);
  Serial.println();
  Serial.print("  LCD Version ID: ");
  Serial.println(lcd_version, HEX);
  if (lcd_version == 0x0000) {
    Serial.println(F("  No LCD version could be read out: no connection"));
    return NOCONN_RESULT;
  }
  if (lcd_version != 0x9325 && lcd_version != 0x9328) {
    // Diagnose the version ID
    if ((lcd_version & 0xFF) == (lcd_version >> 8)) {
      Serial.print(F("  Both version bytes are the same value (0x"));
      Serial.print(lcd_version & 0xFF, HEX);
      Serial.println(F(")\r\n"
                       "  This indicates a possibility that 16-bit data mode is used\r\n"
                       "  Please verify that the 8-bit jumper resistor is in place"));
    }
    // This can be true too...
    Serial.println(F("  Please also check if all control pins are soldered correctly"));

    return TestResult(false, F("Unsupported screen or data pin error"));
  }

  // Perform a register I/O Test using harmless LCD_CMD_GRAM_VER_AD
  Serial.print(F("  Testing LCD register I/O... "));
  const uint32_t total_reg_writes = 100000L;
  for (uint32_t c = 0; c < total_reg_writes; c++) {
    uint16_t w = c & 511;
    PHNDisplayHW::writeRegister(LCD_CMD_GRAM_VER_AD, w);
    uint16_t r = PHNDisplayHW::readRegister(LCD_CMD_GRAM_VER_AD);
    if (w != r) {
      Serial.print(F("Encountered LCD register I/O error after "));
      Serial.print(c);
      Serial.println(F(" register writes"));
      Serial.print(F("  Written: 0x"));
      Serial.print(w, HEX);
      Serial.print(F("  Receive: 0x"));
      Serial.print(r, HEX);
      Serial.println(F("  Screen will have to be replaced."));
      return TestResult(false, F("LCD Register I/O Error"));
    }
  }
  Serial.print(F("Performed "));
  Serial.print(total_reg_writes);
  Serial.println(F(" successful register writes"));

  // Perform test to see if this is an alternative color mode
  Serial.print(F("  Testing pixel CGRAM reading mode... "));
  PHNDisplayHW::setCursor(0, 0);
  PHNDisplay16Bit::writePixel(0x1234);
  uint16_t pixtest_read = PHNDisplay16Bit::readPixel(0, 0);
  if (pixtest_read != 0x1234) {
    Serial.println(F("Could not read back color"));
    Serial.print(F("  Written: 0x"));
    Serial.print(0x1234, HEX);
    Serial.print(F("  Receive: 0x"));
    Serial.print(pixtest_read, HEX);
    Serial.println(F("  Screen will have to be replaced."));
    return TestResult(false, F("LCD CGRAM I/O Read Error"));
  }
  Serial.println("OK");

  // Perform test to see if writing all BLACK works as expected
  Serial.print(F("  Testing pixel CGRAM write-to-black..."));
  for (int i = 0; i < 200; i++) {
    PHNDisplayHW::setCursor(0, 0);
    PHNDisplay8Bit::writePixels(0xFF, PHNDisplayHW::WIDTH*2);
    PHNDisplayHW::setCursor(0, 0);
    PHNDisplay8Bit::writePixels(0x00, PHNDisplayHW::WIDTH);
    for (unsigned int x = 0; x < PHNDisplayHW::WIDTH; x++) {
      color_t pixtest_read = PHNDisplay16Bit::readPixel(x, 0);
      if (pixtest_read != BLACK) {
        Serial.println(F("Could not read back color"));
        Serial.print(F("  Written: 0x0000"));
        Serial.print(F("  Receive: 0x"));
        Serial.print(pixtest_read, HEX);
        Serial.print(F("  X: "));
        Serial.println(x);
        Serial.println(F("  Screen will have to be replaced."));
        return TestResult(false, F("LCD CGRAM I/O Write Error"));
      }
    }
  }
  Serial.println("OK");

  Serial.print(F("  Executing LCD CGRAM I/O Stress test... "));
  PHNDisplayHW::setCursor(0, 0, DIR_RIGHT);
  PHNDisplay16Bit::writePixels(BLACK, PHNDisplayHW::PIXELS);
  color_t test_colors[4] = {RED, GREEN, BLUE, WHITE};
  for (int ctr = 0; ctr < 25; ctr++) {    
    // Draw alternating horizontal lines using alternating bit-pattern
    uint16_t mode_bitpattern = 0x5555;
    for (unsigned int y = 0; y < PHNDisplayHW::HEIGHT/4; y++) {
      PHNDisplay16Bit::writePixels(mode_bitpattern, PHNDisplayHW::WIDTH);
      mode_bitpattern = ~mode_bitpattern;
    }

    // Draw alternating vertical lines
    uint8_t mode_vertbit = 0x00;
    for (uint32_t i = 0; i < PHNDisplayHW::HEIGHT/4*PHNDisplayHW::WIDTH; i++) {
      PHNDisplay8Bit::writePixel(mode_vertbit);
      mode_vertbit = ~mode_vertbit;
    }

    // Draw an alternating cloth
    uint8_t mode_cloth = 0x00;
    for (uint32_t i = 0; i < PHNDisplayHW::HEIGHT/4; i++) {
      mode_cloth = ~mode_cloth;
      for (unsigned int i = 0; i < PHNDisplayHW::WIDTH; i++) {
        PHNDisplay8Bit::writePixel(mode_cloth);
        mode_cloth = ~mode_cloth;
      }
    }

    // Draw alternating RED/GREEN/BLUE/WHITE colors
    for (unsigned int y = 0; y < PHNDisplayHW::HEIGHT/4; y++) {
      for (int c = 0; c < 4; c++) {
        color_t color = test_colors[c];
        for (unsigned int x = 0; x < PHNDisplayHW::WIDTH/4; x++) {
          PHNDisplay16Bit::writePixel(color);
        }
      }
    }

    // Verify we are still synchronized
    color_t c_a = PHNDisplay16Bit::readPixel(PHNDisplayHW::WIDTH-1, PHNDisplayHW::HEIGHT-1);
    color_t c_b = PHNDisplay16Bit::readPixel(0, 0);
    if ((c_a == 0x5555) || (c_b == 0xFFFF)) {
      Serial.println(F("Out of sync."));
      display.fill(BLACK);
      return TestResult(false, F("LCD Stress test failure"));
    }
  }
  Serial.println("OK");

  // Initialize the screen 3 times and show a different test screen each time
  // If initialization fails, the screen will stay white to show it
  color_t screen_colors[3] = {RED, GREEN, BLUE};
  for (int i = 0; i < 3; i++) {
    Serial.print(F("  Displaying test screen #"));
    Serial.print((i+1));
    Serial.println(F(" (check if shown correctly):"));

    PHNDisplayHW::init();
    PHNDisplayHW::setCursor(0, 0);
    for (int y = 0; y < 240; y++) {
      if (i == 2) {
        // Horizontal lines
        color_t c = (y & 0x1) ? screen_colors[i] : BLACK;
        PHNDisplay16Bit::writePixels(c, PHNDisplayHW::WIDTH);
      } else {
        // Vertical lines OR cloth
        boolean odd = (i == 1) && ((y & 0x1) == 0x1);
        int x = 0;
        if (odd) {
          PHNDisplay16Bit::writePixel(screen_colors[i]);
          x += 2;
        }
        do {
          PHNDisplay16Bit::writePixel(BLACK);
          PHNDisplay16Bit::writePixel(screen_colors[i]);
        } while ((x += 2) < 320);
        if (odd) PHNDisplay16Bit::writePixel(BLACK);
      }
    }

    char testText[30];
    strcpy(testText, "Test screen #");
    itoa(i+1, testText+strlen(testText), 10);
    display.setTextColor(WHITE);
    display.drawStringMiddle(0, 0, 320, 240, testText);

    delay(2000);
  }

  display.fill(BLACK);
  return SUCCESS_RESULT;
}

TestResult testTouchscreen() {
  // First check that the touchscreen is NOT touched right now
  if (isScreenTouched()) {
    showMessage("Please release the touchscreen\n"
                "If not touching; indicates failure");

    for (int i = 5; i >= 1; i--) {
      showCounter(i);
      long t = millis();
      while (isScreenTouched() && (millis() - t) < 1000);
    }
    if (isScreenTouched()) {
      return TestResult(false, F("Touch is always detected"));
    }
  }

  // Wait until the screen is touched
  showMessage("Please touch the touchscreen\n"
              "Press in the MIDDLE of the screen");
  for (int i = 5; i >= 1; i--) {
    showCounter(i);

    long t = millis();
    while ((millis() - t) < 1000) {
      uint16_t x, y, z1, z2;
      PHNDisplayHW::readTouch(&x, &y, &z1, &z2);
      if (z1 == 0) {
        continue;
      }

      Serial.print(F("  TOUCH: ["));
      Serial.print(x); Serial.print(", ");
      Serial.print(y); Serial.print(", ");
      Serial.print(z1); Serial.print(", ");
      Serial.print(z2); Serial.println("]");

      if (x <= 200 || x >= 800 || y <= 200 || y >= 800) {
        return TestResult(false, F("Read touch was out of bounds"));
      }

      return SUCCESS_RESULT;
    }
  }

  // Timeout - fail
  return TestResult(false, F("Touch is never detected"));
}

TestResult testFlash() {
  // Checks if the test routine flash is read back correctly by comparing CRC values of test routine flash
  uint32_t testroutine_crc_check = calcCRCFlash(0, testroutine_length, 0);
  String txt_crc_a(testroutine_crc, HEX);
  String txt_crc_b(testroutine_crc_check, HEX);
  txt_crc_a.toUpperCase();
  txt_crc_b.toUpperCase();
  if (testroutine_crc != testroutine_crc_check) {
    String errorStr;
    errorStr += "Verification error: CRC ";
    errorStr += txt_crc_a;
    errorStr += " != ";
    errorStr += txt_crc_b;
    return TestResult(false, errorStr);
  }

  // All good!
  String succStr;
  succStr += "Verification OK: CRC ";
  succStr += txt_crc_a;
  return TestResult(true, succStr);
}

TestResult testConnector() {
  // Wait with timeout for a token response to be read back
  if (!isStationConnected) {
    return TestResult(true, "Skipped: requires test station");
  }
  
  // In here is a polling response system for performing the tests
  // No fancy protocol; just raw communication
  char buff[200];
  int buff_idx = 0;
  long timeout_start = millis();
  while (true) {
    // Wait for data with a timeout
    while (!Serial.available()) {
      if ((millis() - timeout_start) > 500) {
        return TestResult(false, F("Test station Serial timeout"));
      }
    }
    timeout_start = millis();
    
    char c = Serial.read();
    
    // Process the message on every newline
    if (c == '\n' || c == '\r') {
      buff[buff_idx] = 0;
      
      boolean setting = !memcmp(buff, "SET", 3);
      if (setting || !memcmp(buff, "GET", 3)) {
        char mode = buff[3];
        int pin = atoi(buff + 4);
        if (setting) {
          if (mode == 'L' || mode == 'H') {
            pinMode(pin, OUTPUT);
          } else {
            pinMode(pin, INPUT);
          }
          if (mode == 'H' || mode == 'P') {
            digitalWrite(pin, HIGH);
          } else {
            digitalWrite(pin, LOW);
          }
        } else {
          if (digitalRead(pin)) {
            Serial.print('H');
          } else {
            Serial.print('L');
          }
        }
      } else if (!memcmp(buff, "FAIL", 4)) {
        char message[220];
        strcpy(message, "Failure of pin(s) ");
        strcpy(message + strlen(message), buff + 4);
        return TestResult(false, message);
      } else if (!memcmp(buff, "PASS", 4)) {
        return SUCCESS_RESULT;
      }
      buff_idx = 0;
    } else {
      buff[buff_idx++] = c;
    }
  }
  
}

TestResult testBMP180() {
  SFE_BMP180 barometer;
  if (!barometer.begin()) {
    return NOCONN_RESULT;
  }
  char status;
  
  // Perform several measurements of temperature and pressure
  // Check if all goes well, then do a variance check as well
  const int MEAS_CNT = 10;
  int16_t temp_values[MEAS_CNT];
  int16_t press_values[MEAS_CNT];
  for (int i = 0; i < MEAS_CNT; i++) {
    // Short delay
    delay(100);
    
    // Measure and validate the temperature
    double temperature;
    status = barometer.startTemperature();
    if (!status) return TestResult(false, F("Failed to measure temperature"));
    delay(status);
    barometer.getTemperature(temperature);
    if (temperature < -40.0 || temperature >= 85.0) {
      Serial.print("TEMP EXCEEDS: ");
      Serial.print(temperature);
      return TestResult(false, F("Temperature exceeds sensor limits"));
    }
    temp_values[i] = (int16_t) temperature;

    // Measure and validate the pressure in millibar
    double pressure;
    status = barometer.startPressure(1);
    if (!status) return TestResult(false, F("Failed to measure pressure"));
    delay(status);
    barometer.getPressure(pressure, temperature);
    pressure *= 10.0; // Convert to Pascals
    if (pressure <= 8000.0 || pressure >= 12000) {
      Serial.print("PRESSURE EXCEEDS: ");
      Serial.print(pressure);
      Serial.print("Pa");
      return TestResult(false, F("Pressure exceeds sensor limits"));
    }
    press_values[i] = (int16_t) pressure;
  }
  
  // Calculate statistics
  int32_t temp_mean, temp_dev, press_mean, press_dev;
  calc_stats(temp_values, MEAS_CNT, &temp_mean, &temp_dev);
  calc_stats(press_values, MEAS_CNT, &press_mean, &press_dev);
  
  // Print the measurement data
  Serial.println();
  Serial.print(F("  TEMPERATURE: "));
  print_sensor_info(&temp_mean, &temp_dev, 1);
  Serial.print(F("  PRESSURE: "));
  print_sensor_info(&press_mean, &press_dev, 1);

  // Check for out of bounds data
  if (abs(temp_dev) > 3) return TestResult(false, F("Temperature readout is unstable"));
  if (abs(press_dev) > 3) return TestResult(false, F("Pressure readout is unstable"));

  // All good!
  return SUCCESS_RESULT;
}

TestResult testMPU6050() {
  MPU6050 accelgyro(MPU6050_ADDRESS_AD0_LOW);  
  Wire.begin();
  accelgyro.initialize();
  if (!accelgyro.testConnection()) {
    return NOCONN_RESULT;
  }

  // Generate 5 samples of accelerometer and gyro data
  const int MEAS_CNT = 5;
  int16_t accel_x[MEAS_CNT];
  int16_t accel_y[MEAS_CNT];
  int16_t accel_z[MEAS_CNT];
  int16_t gyro_x[MEAS_CNT];
  int16_t gyro_y[MEAS_CNT];
  int16_t gyro_z[MEAS_CNT];
  for (int i = 0; i < MEAS_CNT; i++) {
    delay(100);
    accelgyro.getAcceleration(&accel_x[i], &accel_y[i], &accel_z[i]);
    accelgyro.getRotation(&gyro_x[i], &gyro_y[i], &gyro_z[i]);
  }

  // Generate mean and deviation information from the gathered data
  int32_t accel_mean[3];
  int32_t accel_dev[3];
  int32_t gyro_mean[3];
  int32_t gyro_dev[3];
  calc_stats(accel_x, MEAS_CNT, &accel_mean[0], &accel_dev[0]);
  calc_stats(accel_y, MEAS_CNT, &accel_mean[1], &accel_dev[1]);
  calc_stats(accel_z, MEAS_CNT, &accel_mean[2], &accel_dev[2]);
  calc_stats(gyro_x, MEAS_CNT, &gyro_mean[0], &gyro_dev[0]);
  calc_stats(gyro_y, MEAS_CNT, &gyro_mean[1], &gyro_dev[1]);
  calc_stats(gyro_z, MEAS_CNT, &gyro_mean[2], &gyro_dev[2]);

  // Print debug information out to Serial
  Serial.println();
  Serial.print(F("  ACCEL: "));
  print_sensor_info(accel_mean, accel_dev, 3);
  Serial.print(F("  GYRO: "));
  print_sensor_info(gyro_mean, gyro_dev, 3);

  // Check that the mean and deviation is within acceptable limits
  // A deviation of 0 is nearly impossible - there is always some noise expected
  // Accelerometer
  for (int i = 0; i < 3; i++) {
    if (accel_dev[i] == 0 || accel_dev[i] > 1000) {
      return TestResult(false, F("Accelerometer deviation out of bounds"));
    }
    if (abs(accel_mean[i]) == 16000) {
      return TestResult(false, F("Accelerometer mean indicates failure"));
    }
  }
  // Gyrometer
  for (int i = 0; i < 3; i++) {
    if (gyro_dev[i] == 0 || gyro_dev[i] > 500) {
      return TestResult(false, F("Gyrometer deviation out of bounds"));
    }
    if (abs(gyro_mean[i]) >= 500) {
      return TestResult(false, F("Gyrometer mean out of bounds"));
    }
  }
  return SUCCESS_RESULT;
}

TestResult testHMC5883L() {
  HMC5883L compass;
  
  // Initialize it
  Wire.begin();
  compass.initialize();
  
  // Make sure the sensor connection is working
  if (!compass.testConnection()) {
    return NOCONN_RESULT;
  }

  // Perform a series of measurements
  const int MEAS_CNT = 10;
  int16_t mag_x[MEAS_CNT];
  int16_t mag_y[MEAS_CNT];
  int16_t mag_z[MEAS_CNT];
  for (int i = 0; i < MEAS_CNT; i++) {
    compass.getHeading(&mag_x[i], &mag_y[i], &mag_z[i]);
    delay(25);
  }

  // Perform magical statistics
  int32_t mag_mean[3];
  int32_t mag_dev[3];
  calc_stats(mag_x, MEAS_CNT, &mag_mean[0], &mag_dev[0]);
  calc_stats(mag_y, MEAS_CNT, &mag_mean[1], &mag_dev[1]);
  calc_stats(mag_z, MEAS_CNT, &mag_mean[2], &mag_dev[2]);

  // Print out the measurement data
  Serial.println();
  Serial.print("  MAGNETO: ");
  print_sensor_info(mag_mean, mag_dev, 3);

  // Perform a check to see if the measured data is within range
  for (int i = 0; i < 3; i++) {
    if (mag_dev[i] >= 200) {
      return TestResult(false, F("Readings are unstable"));
    }
    if (mag_mean[i] >= 2000 || mag_mean[i] <= -2000) {
      return TestResult(false, F("Readings out of range/magnetic influence"));
    }
  }

  // All good
  return SUCCESS_RESULT;
}

TestResult testRAM() {
  // De-init first
  SPI.end();

  // Initialize SPI and the chip
  sram.begin();

  // Write different values to different areas in memory
  // Read them back to verify it works
  for (uint16_t addr = 20; addr < 60000; addr += 555) {
    // First do a 0xFF pass
    if (!SpiRAMTest(addr, 0xFF)) {
      String msg = "Wiping pass failed: addr=";
      msg += addr;
      return TestResult(false, msg);
    }

    // Then write a value based on the address
    if (!SpiRAMTest(addr, addr & 0xFF)) {
      String msg = "Data verification failed: addr=";
      msg += addr;
      return TestResult(false, msg);
    }
  }
  return SUCCESS_RESULT;
}

TestResult testSIM() {
  // Test whether the SIM908 powered on
  if (!sim.isOn()) {
    return TestResult(false, F("Failed to turn on SIM (PWR pin?)"));
  }
  // Try to send an AT command with several retry attempts
  // Turn off SIM when done
  for (int i = 0; i < 2 && !sim.writeATCommand("AT"); i++);
  bool atSucc = sim.writeATCommand("AT");
  sim.end();
  if (!atSucc) {
    return TestResult(false, F("No response to AT-Command"));
  }

  // Test out some of the control pins
  if (!doPinPullHighTest(SIM_STATUS_PIN)) {
    return TestResult(false, F("Status pin disconnected."));
  }
  if (!doPinPullHighTest(SIM_DTRS_PIN)) {
    return TestResult(false, F("DTRS pin disconnected."));
  }
  if (!doPinPullHighTest(SIM_RI_PIN)) {
    return TestResult(false, F("RI pin disconnected."));
  }
  return SUCCESS_RESULT;
}

TestResult testWiFi() {
  // Probe the AT Command and switch up the baud rate until it works
  boolean found_baud = false;
  for (int baud_idx = 0; baud_idx < baud_rates_cnt; baud_idx++) {
    disableBluetoothWiFi();
    delay(50);
    enableWiFi();
    set_BlueWifi_baud(baud_rates[baud_idx]);

    if (writeATCommand(Serial2, "AT\r\n", "OK", 250)) {
      found_baud = true;
      break;
    }
  }
  if (!found_baud) {
    return TestResult(false, F("No response to AT-Command"));
  }
  return SUCCESS_RESULT;
}

TestResult testBluetooth() {
  // Turn on the bluetooth module
  enableBluetooth();

  // First wake up the device and probe with an AT-command
  if (!writeATCommand(Serial2, "AT", "OK", 250)) {
    // In case a connection is currently open, clear it
    pinMode(BLUETOOTH_KEY_PIN, OUTPUT);
    digitalWrite(BLUETOOTH_KEY_PIN, LOW);
    delay(2000);
    digitalWrite(BLUETOOTH_KEY_PIN, HIGH);
    
    // Probe the AT Command and switch up the baud rate until it works
    boolean found_baud = false;
    for (int baud_idx = 0; baud_idx < baud_rates_cnt; baud_idx++) {
      disableBluetoothWiFi();
      delay(10);
      enableBluetooth();
      set_BlueWifi_baud(baud_rates[baud_idx]);
      
      if (writeATCommand(Serial2, "AT", "OK", 500)) {
        found_baud = true;
        break;
      }
    }
    if (!found_baud) {
      return TestResult(false, F("No response to AT-Command"));
    }
  }

  // Reset to factory defaults
  if (!writeATCommand(Serial2, "AT+RENEW", "OK+RENEW", 200)) {
    return TestResult(false, F("Failed to restore factory defaults"));
  }

  // Reset the device
  if (!writeATCommand(Serial2, "AT+RESET", "OK+RESET", 200)) {
    return TestResult(false, F("Failed to reset the module"));
  }

  return SUCCESS_RESULT;
}

TestResult testSD() {
  // Attempt to initialize the Micro-SD
  volume.isInitialized = 0;
  if (!volume_init()) {
    return TestResult(false, F("Failed to initialize SD card"));
  }

  // Attempt to find the SKETCHES.HEX
  if (!file_open("SKETCHES", "HEX", SDMIN_FILE_READ)) {
    return TestResult(false, F("Unable to find main sketch"));
  }

  // In test station mode, set sketch to read-only
  if (isStationConnected) {
    file_readCacheDir()->attributes |= DIR_ATT_READ_ONLY;
    volume_writeCache();
  }

  // Make sure to de-initialize the Micro-SD CS pin
  card_setEnabled(false);

  // All good
  return TestResult(true, "Accessible; No problem found");
}

TestResult testMIDI() {
  showMessage("Plug in headphones - hear chimes?\n"
              "Press SELECT if you hear sound");

  PHN_Midi midi;
  midi.begin();
  
  for (int i = 5; i >= 1; i--) {
    showCounter(i);

    char notes[] = {40, 41, 42, 43, 44, 45, 35};
    for (uint8_t i = 0; i < sizeof(notes); i++) {
      midi.noteOn(0, notes[i], 120);
      
      long t = millis();
      while ((millis() - t) < 150) {
        if (isSelectPressed()) {
          midi.noteOff(0, notes[i], 50);
          return TestResult(true, "User indicated successful");
        }
      }
      midi.noteOff(0, notes[i], 120);
    }
  }

  // Failure - no SELECT pressed
  return TestResult(false, F("User indicated no sound"));
}

TestResult testMP3() {
  // Define and initialize the VS1053 audio player here
  Adafruit_VS1053_FilePlayer musicPlayer(VS1053_RESET_PIN, VS1053_CS_PIN, VS1053_DCS_PIN, VS1053_DREQ_PIN, VS1053_CARDCS_PIN);
  
  // Initialize the VS1053 digital pins
  pinMode(VS1053_GPIO_PIN, OUTPUT);
  pinMode(VS1053_PWR_PIN, OUTPUT);
  pinMode(VS1053_DCS_PIN, OUTPUT);
  digitalWrite(VS1053_PWR_PIN, HIGH);
  digitalWrite(VS1053_GPIO_PIN, LOW);
  digitalWrite(VS1053_DCS_PIN, LOW);

  // Initialize the VS1053 library
  if (!musicPlayer.begin()) {
    return TestResult(false, F("Failed to initialize VS1053 chip"));
  }

  // Make sure to use the pin interrupt
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);

  // Initialize the SD library for file playback
  // [!] This has to be done AFTER the initialization of the VS1053 [!]
  if (!SD.begin(VS1053_CARDCS_PIN)) {
    return TestResult(false, F("Failed to initialize SD card"));
  }

  // Check if the test sound file exists
  char testSoundFile[100] = "Sounds/beep.mp3";
  if (!SD.exists(testSoundFile)) {
    return TestResult(false, F("Micro-SD has no Sounds/beep.mp3"));
  }

  showMessage("Plug in headphones - hear beeps?\n"
              "Press SELECT if you hear sound");

  // Play the test 'beep' file on repeat
  bool success = false;
  for (int i = 5; i >= 1 && !success; i--) {
    showCounter(i);

    musicPlayer.startPlayingFile(testSoundFile);

    long t = millis();
    while ((millis() - t) < 900) {
      if (isSelectPressed()) {
        success = true;
        break;
      }
    }
  }

  // Stop music player before exiting (!)
  musicPlayer.stopPlaying();
  delay(200);

  // Exit with success state
  if (success) {
    return TestResult(true, "User indicated successful");
  } else {
    return TestResult(false, F("User indicated no sound"));
  }
}

TestResult testNone() {
  return TestResult(true, "No Test Performed");
}

