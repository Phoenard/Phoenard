#include "Phoenard.h"
#include <Wire.h>
#include <HMC5883L.h>

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
  compass.SetMeasurementMode(Measurement_Continuous);

  // Initialize and add the widgets
  color_t bar_colors[3] = {RED, GREEN, BLUE};
  for (int i = 0; i < 3; i++) {
    bars[i].setBounds(10 + i * 25, 20, 23, 200);
    bars[i].setRange(0.0F, 4096.0F);
    bars[i].setColor(CONTENT, bar_colors[i]);
    display.addWidget(bars[i]);
  }

  // Draw the compass circle
  display.fillCircle(compass_x, compass_y, compass_rad, WHITE);
  display.drawCircle(compass_x, compass_y, compass_rad - 1, BLACK);
}

void loop() {
  // Retrive the raw values from the compass (not scaled).
  MagnetometerRaw raw = compass.ReadRawAxis();

  // Obtain the x/y/z values
  float x = (float) abs(raw.ZAxis);
  float y = (float) abs(raw.YAxis);
  float z = (float) abs(raw.ZAxis);
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
