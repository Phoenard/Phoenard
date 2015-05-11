#include "Phoenard.h"
#include <Wire.h>
#include <BMP085.h>

// Define the sensor
// Note: BMP085 library works for both BMP085 and BMP180
BMP085 dps = BMP085();

// Define the widgets used to display things
PHN_Label tempLabel;
PHN_Gauge tempGauge;
PHN_Label altLabel;
PHN_BarGraph altBar;
PHN_Label altValueLabel;
PHN_Label pressureLabel;
PHN_BarGraph pressureBar;
PHN_Label pressureValueLabel;

// Used for filtering the altitude data
float alt_filter = 500.0F;

void setup(void) {
  Wire.begin();
  dps.init(MODE_ULTRA_HIGHRES, 500, true);
  
  // Add temperature label
  tempLabel.setBounds(40, 70, 80, 20);
  tempLabel.setText("Temperature:");
  display.addWidget(tempLabel);
  
  // Add temperature gauge
  tempGauge.setBounds(130, 20, 100, 100);
  tempGauge.setRange(0.0F, 40.0F);
  display.addWidget(tempGauge);
  
  // Add altitude label
  altLabel.setBounds(50, 150, 60, 20);
  altLabel.setText("Altitude:");
  display.addWidget(altLabel);
  
  // Add altitude bar graph
  altBar.setBounds(130, 140, 100, 30);
  altBar.setRange(0.0F, 1000.0F);
  display.addWidget(altBar);
  
  // Add altitude value label
  altValueLabel.setBounds(240, 150, 60, 20);
  display.addWidget(altValueLabel);
  
  // Add pressure label
  pressureLabel.setBounds(50, 190, 60, 20);
  pressureLabel.setText("Pressure:");
  display.addWidget(pressureLabel);
  
  // Add pressure bar graph
  pressureBar.setBounds(130, 180, 100, 30);
  pressureBar.setRange(90000.0F, 110000.0F);
  display.addWidget(pressureBar);
  
  // Add pressure value label
  pressureValueLabel.setBounds(240, 190, 60, 20);
  display.addWidget(pressureValueLabel);
}

void loop(void) {
  long Temperature = 0, Pressure = 0, Altitude = 0;
  dps.getTemperature(&Temperature); 
  dps.getPressure(&Pressure);
  dps.getAltitude(&Altitude);
  alt_filter += 0.1 * ((float) Altitude - alt_filter);

  tempGauge.setValue(0.1 * Temperature);
  altBar.setValue(alt_filter);
  pressureBar.setValue(Pressure);
  
  String altText;
  altText += (int) alt_filter;
  altText += " cm";
  altValueLabel.setText(altText);
  
  String pressText;
  pressText += Pressure;
  pressText += " Pa";
  pressureValueLabel.setText(pressText);

  display.update();
}