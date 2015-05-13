/*
 * Magnetometer test application that displays the readings
 * and calculated heading on the screen.
 *
 * Makes use of the i2cdev HMC5883L library:
 * https://github.com/jrowberg/i2cdevlib/tree/master/Arduino/HMC5883L
 */
#include "Phoenard.h"
#include <Wire.h>
#include "I2Cdev.h"
#include "HMC5883L.h"

// Define the compass
HMC5883L compass;

// Define bars to show the axis on
PHN_BarGraph bars[3];

// Variables describing the compass
int compass_rad = (display.height() - 40) / 2;
int compass_y = display.height() / 2;
int compass_x = display.width() - compass_y;
float compass_oldAngle = 0.0F;

void setup() {
  // Initialize and set up the I2C connection / compass
  Wire.begin();
  compass.initialize();
  if (!compass.testConnection()) {
    display.fill(RED);
    while (true) {}
  }

  // Initialize and add the widgets
  color_t bar_colors[3] = {RED, GREEN, BLUE};
  for (int i = 0; i < 3; i++) {
    bars[i].setBounds(10 + i * 25, 20, 23, 200);
    bars[i].setRange(-4096.0F, 4096.0F);
    bars[i].setBaseValue(0.0F);
    bars[i].setColor(CONTENT, bar_colors[i]);
    display.addWidget(bars[i]);
  }

  // Draw the compass circle
  display.fillCircle(compass_x, compass_y, compass_rad, WHITE);
  display.drawCircle(compass_x, compass_y, compass_rad - 1, BLACK);
}

void loop() {
  // Retrive the raw values from the compass (not scaled).
  int16_t x, y, z;
  compass.getHeading(&x, &y, &z);

  // Set the three bars to show the raw data
  bars[0].setValue(x);
  bars[1].setValue(y);
  bars[2].setValue(z);

  // Obtain only two axis of interest to calculate an angle
  float angle = atan2(x, y);
  
  // Draw the angle pointer on screen
  if (compass_oldAngle != angle) {
    drawPointer(compass_oldAngle, WHITE);
    drawPointer(angle, BLACK);
    compass_oldAngle = angle;
  }

  display.update();
  delay(70);
}

void drawPointer(float angle, color_t color) {
  int dx = (int) ((float) (compass_rad - 2) * cos(angle));
  int dy = (int) ((float) (compass_rad - 2) * sin(angle));
  display.drawLine(compass_x + dx, compass_y + dy, compass_x, compass_y, color);
}
