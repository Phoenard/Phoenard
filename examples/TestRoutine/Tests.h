#include "Phoenard.h"

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

typedef struct TestResult {
  boolean success;
  char device[20];
  char status[50];
  
  TestResult() {
    success = false;
  }
  TestResult(const TestResult& result) {
    this->success = result.success;
    memcpy(this->status, result.status, sizeof(status));
    memcpy(this->device, result.device, sizeof(device));
  }
  TestResult(boolean success, char* status) {
    this->success = success;
    memcpy(this->status, status, strlen(status) + 1);
  }
  TestResult(boolean success, String status) {
    this->success = success;
    status.toCharArray(this->status, sizeof(this->status));
  }
} TestResult;

TestResult SUCCESS_RESULT(true, "No problems found");
TestResult NOCONN_RESULT(false, "No connection with device");

boolean isStationConnected = false;

boolean readToken(Stream &serial, char *token, long timeoutMS) {
  uint16_t i = 0;
  long startTime = millis();
  while (token[i]) {
    if ((millis() - startTime) >= timeoutMS) {
      // Timeout
      return false;
    }
    if (serial.available() && serial.read() != token[i++]) i = 0;
  }
  return true;
}

boolean writeATCommand(Stream &serial, char *command, char* response, long timeoutMS) {
  for (int i = 0; i < 3; i++) {
    serial.print(command);
    if (readToken(serial, response, timeoutMS)) return true;
  }
  return false;
}

void set_BlueWifi_baud(long baud) {
  Serial.println();
  Serial.print("  SETTING BAUD=");
  Serial.print(baud);
  Serial.print(" | ");
  Serial2.begin(baud);
}

bool SpiRAMTest(uint16_t address, char data_byte) {
  sram.write(address, data_byte);
  char result = sram.read(address);
  if (result == data_byte) return true;

  Serial.print("Data Error at address [");
  Serial.print(address);
  Serial.print("]: ");
  Serial.print(result);
  Serial.print(" != ");
  Serial.print((uint8_t) data_byte);
  Serial.print(" ");
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

boolean isScreenTouched() {
  uint16_t x, y;
  float pressure;
  PHNDisplayHW::readTouch(&x, &y, &pressure);
  return pressure >= PHNDisplayHW::PRESSURE_THRESHOLD;
}

/** ========================================================== **/

TestResult testScreen() {
  // First try to read the LCD version ID
  // This provides a basic check for the RS/WR/data pins
  uint16_t lcd_version = PHNDisplayHW::readRegister(0);
  Serial.println();
  Serial.print("  LCD Version ID: ");
  Serial.println(lcd_version, HEX);
  if (lcd_version == 0x0000) {
    return NOCONN_RESULT;
  }
  if (lcd_version != 0x9325 && lcd_version != 0x9328) {
    return TestResult(false, "Unsupported screen or data pin error");
  }

  // Perform pixel read/write testing on the top pixel line
  // This tests every pixel and is slow, so doesn't test many pixels
  const uint32_t total_lines = 100;
  const uint32_t total_writes = PHNDisplayHW::WIDTH * total_lines;
  uint16_t color_write = 1;
  uint16_t color_read;
  uint32_t error_cnt = 0;
  for (int line = 0; line < total_lines; line++) {
    // Wipe the line with all 0
    PHNDisplay16Bit::drawLine(0, 0, PHNDisplayHW::WIDTH, DIR_RIGHT, 0x0000);

    PHNDisplayHW::setCursor(0, 0, DIR_RIGHT);
    color_write = 0x5555;
    for (uint16_t x = 0; x < PHNDisplayHW::WIDTH; x++) {
      PHNDisplay16Bit::writePixel(color_write);
      color_write ^= 0xFFFF;
      color_write++;
    }

    // Read the line back
    color_write = 0x5555;
    for (uint16_t x = 0; x < PHNDisplayHW::WIDTH; x++) {
      color_read = PHNDisplay16Bit::readPixel(x, 0);
      if (color_read != color_write) {
        error_cnt++;
      }
      color_write ^= 0xFFFF;
      color_write++;
    }
  }

  // Clear the top line again
  PHNDisplay16Bit::drawLine(0, 0, PHNDisplayHW::WIDTH, DIR_RIGHT, BLACK);

  // Log test information
  Serial.print("  R/W Errors: ");
  Serial.print(error_cnt);
  Serial.print(" (");
  Serial.print(total_writes);
  Serial.println(" Writes)");
  if (error_cnt) {
    char respText[50];
    itoa(error_cnt, respText, 10);
    strcat(respText, " data read/write errors");
    return TestResult(false, respText);
  }

  // Perform full-line write testing within a viewport
  // This pushes out a huge amount of pixels in an attempt to get out of sync
  Serial.print("  Sync test: ");
  color_write = 0x55AA;
  const uint32_t full_total_lines = 1000;
  const uint32_t full_total_pixels = (uint32_t) full_total_lines * (uint32_t) PHNDisplayHW::WIDTH;
  PHNDisplayHW::setViewport(0, 0, PHNDisplayHW::WIDTH-1, 0);
  PHNDisplayHW::setCursor(0, 0);
  for (int i = 0; i < full_total_lines; i++) {
    color_write ^= 0xFFFF;
    color_write++;
    PHNDisplay16Bit::writePixels(color_write, PHNDisplayHW::WIDTH);
  }
  PHNDisplayHW::setViewport(0, 0, PHNDisplayHW::WIDTH-1, PHNDisplayHW::HEIGHT-1);
  color_read = PHNDisplay16Bit::readPixel(PHNDisplayHW::WIDTH-1, 0);
  if (color_write != color_read) {
    Serial.print("Out of sync after ");
    Serial.print(full_total_pixels);
    Serial.print(" pixel writes");
    return TestResult(false, "Pixel writing out of sync");
  } else {
    Serial.print("All ");
    Serial.print(full_total_pixels);
    Serial.println(" pixels written while staying synchronized");
  }

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
      return TestResult(false, "Touch is always detected");
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

      Serial.print("  TOUCH: [");
      Serial.print(x); Serial.print(", ");
      Serial.print(y); Serial.print(", ");
      Serial.print(z1); Serial.print(", ");
      Serial.print(z2); Serial.println("]");

      if (x <= 200 || x >= 800 || y <= 200 || y >= 800) {
        return TestResult(false, "Read touch was out of bounds");
      }

      return SUCCESS_RESULT;
    }
  }

  // Timeout - fail
  return TestResult(false, "Touch is never detected");
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
        return TestResult(false, "Test station Serial timeout");
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
    if (!status) return TestResult(false, "Failed to measure temperature");
    delay(status);
    barometer.getTemperature(temperature);
    if (temperature < -40.0 || temperature >= 85.0) {
      Serial.print("TEMP EXCEEDS: ");
      Serial.print(temperature);
      return TestResult(false, "Temperature exceeds sensor limits");
    }
    temp_values[i] = (int16_t) temperature;

    // Measure and validate the pressure in millibar
    double pressure;
    status = barometer.startPressure(1);
    if (!status) return TestResult(false, "Failed to measure pressure");
    delay(status);
    barometer.getPressure(pressure, temperature);
    pressure *= 10.0; // Convert to Pascals
    if (pressure <= 8000.0 || pressure >= 12000) {
      Serial.print("PRESSURE EXCEEDS: ");
      Serial.print(pressure);
      Serial.print("Pa");
      return TestResult(false, "Pressure exceeds sensor limits");
    }
    press_values[i] = (int16_t) pressure;
  }
  
  // Calculate statistics
  int32_t temp_mean, temp_dev, press_mean, press_dev;
  calc_stats(temp_values, MEAS_CNT, &temp_mean, &temp_dev);
  calc_stats(press_values, MEAS_CNT, &press_mean, &press_dev);
  
  // Print the measurement data
  Serial.println();
  Serial.print("  TEMPERATURE: ");
  print_sensor_info(&temp_mean, &temp_dev, 1);
  Serial.print("  PRESSURE: ");
  print_sensor_info(&press_mean, &press_dev, 1);

  // Check for out of bounds data
  if (abs(temp_dev) > 3) return TestResult(false, "Temperature readout is unstable");
  if (abs(press_dev) > 3) return TestResult(false, "Pressure readout is unstable");

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
  Serial.print("  ACCEL: ");
  print_sensor_info(accel_mean, accel_dev, 3);
  Serial.print("  GYRO: ");
  print_sensor_info(gyro_mean, gyro_dev, 3);

  // Check that the mean and deviation is within acceptable limits
  // A deviation of 0 is nearly impossible - there is always some noise expected
  // Accelerometer
  for (int i = 0; i < 3; i++) {
    if (accel_dev[i] == 0 || accel_dev[i] > 1000) {
      return TestResult(false, "Accelerometer deviation out of bounds");
    }
    if (abs(accel_mean[i]) == 16000) {
      return TestResult(false, "Accelerometer mean indicates failure");
    }
  }
  // Gyrometer
  for (int i = 0; i < 3; i++) {
    if (gyro_dev[i] == 0 || gyro_dev[i] > 500) {
      return TestResult(false, "Gyrometer deviation out of bounds");
    }
    if (abs(gyro_mean[i]) >= 500) {
      return TestResult(false, "Gyrometer mean out of bounds");
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
      return TestResult(false, "Readings are unstable");
    }
    if (mag_mean[i] >= 2000 || mag_mean[i] <= -2000) {
      return TestResult(false, "Readings out of range/magnetic influence");
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
    return TestResult(false, "Failed to turn on SIM");
  }
  // Try to send an AT command with several retry attempts
  for (int i = 0; i < 2 && !sim.writeATCommand("AT"); i++);
  if (!sim.writeATCommand("AT")) {
    return TestResult(false, "No response to AT-Command");
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
    return TestResult(false, "No response to AT-Command");
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
      return TestResult(false, "No response to AT-Command");
    }
  }

  // Reset to factory defaults
  if (!writeATCommand(Serial2, "AT+RENEW", "OK+RENEW", 200)) {
    return TestResult(false, "Failed to restore factory defaults");
  }

  // Reset the device
  if (!writeATCommand(Serial2, "AT+RESET", "OK+RESET", 200)) {
    return TestResult(false, "Failed to reset the module");
  }

  return SUCCESS_RESULT;
}

TestResult testSD() {
  // Attempt to initialize the Micro-SD
  volume.isInitialized = 0;
  if (!volume_init()) {
    return TestResult(false, "Failed to initialize SD card");
  }

  // Attempt to find the SKETCHES.HEX
  if (!file_open("SKETCHES", "HEX", SDMIN_FILE_READ)) {
    return TestResult(false, "Unable to find main sketch");
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
  return TestResult(false, "User indicated no sound");
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
  musicPlayer.begin();

  // Make sure to use the pin interrupt
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);

  // Initialize the SD library for file playback
  // [!] This has to be done AFTER the initialization of the VS1053 [!]
  if (!SD.begin(VS1053_CARDCS_PIN)) {
    return TestResult(false, "Failed to initialize SD card");
  }

  // Check if the test sound file exists
  if (!SD.exists("Sounds/beep.mp3")) {
    return TestResult(false, "Micro-SD has no Sounds/beep.mp3");
  }

  showMessage("Plug in headphones - hear beeps?\n"
              "Press SELECT if you hear sound");

  // Play the test 'beep' file on repeat
  for (int i = 5; i >= 1; i--) {
    showCounter(i);

    musicPlayer.startPlayingFile("Sounds/beep.mp3");

    long t = millis();
    while ((millis() - t) < 900) {
      if (isSelectPressed()) {
        return TestResult(true, "User indicated successful");
      }
    }
  }

  // Failure - no SELECT pressed
  return TestResult(false, "User indicated no sound");
}
