#include "Phoenard.h"
#include <Wire.h>
#include <SFE_BMP180.h>

// Define the sensor
SFE_BMP180 barometer = SFE_BMP180();

// Define the widgets used to display things
PHN_Label tempLabel;
PHN_Gauge tempGauge;
PHN_Label altLabel;
PHN_BarGraph altBar;
PHN_Label altValueLabel;
PHN_Label pressureLabel;
PHN_BarGraph pressureBar;
PHN_Label pressureValueLabel;

// Altitude at your current location (please set)
double ALTITUDE_BASE = 0;

// First measurement defines the base pressure for relative altitude
double Pressure_relBase;
boolean Pressure_relBase_calc = false;

// Filtered relative altitude value
double Altitude_relative_filt = 0.0;

void setup(void) {
  Wire.begin();
  barometer.begin();
  Serial.begin(9600);
  
  // Add temperature label
  tempLabel.setBounds(32, 70, 80, 20);
  tempLabel.setText("Temperature (C):");
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
  altBar.setRange(-10.0F, 10.0F);
  altBar.setColor(CONTENT, BLUE);
  display.addWidget(altBar);
  
  // Add altitude value label
  altValueLabel.setBounds(240, 140, 60, 30);
  display.addWidget(altValueLabel);
  
  // Add pressure label
  pressureLabel.setBounds(50, 190, 60, 20);
  pressureLabel.setText("Pressure:");
  display.addWidget(pressureLabel);
  
  // Add pressure bar graph
  pressureBar.setBounds(130, 180, 100, 30);
  pressureBar.setRange(0.0F, 2000.0F);
  pressureBar.setColor(CONTENT, BLUE);
  display.addWidget(pressureBar);
  
  // Add pressure value label
  pressureValueLabel.setBounds(240, 180, 60, 30);
  display.addWidget(pressureValueLabel);
  
  // Wait until initialized
  delay(barometer.startTemperature());
}

void loop(void) {
  double Temperature;
  double Pressure;
  double Pressure_seaLevel;
  double Altitude;
  double Altitude_relative;

  /*
   * Do the temperature measurement
   * startTemperature() is 0 when an error occurs, otherwise is time to wait
   */
  delay(barometer.startTemperature());
  barometer.getTemperature(Temperature);

  /*
   * Do the pressure measurement
   * The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
   * startPressure() is 0 when an error occurs, otherwise is time to wait
   */
  delay(barometer.startPressure(3));
  barometer.getPressure(Pressure, Temperature);
  if (!Pressure_relBase_calc) {
    Pressure_relBase_calc = true;
    Pressure_relBase = Pressure;
  }

  /*
   * Measure a rough relative altitude between different pressure values
   * Because it fluctuates a lot, a filter is introduced
   */
  Altitude_relative = barometer.altitude(Pressure, Pressure_relBase);
  Altitude_relative_filt += 0.02 * (Altitude_relative - Altitude_relative_filt);

  /*
   * Convert the pressure into an altitude estimate
   * This is done by first converting to a sea level pressure
   */
  Pressure_seaLevel = barometer.sealevel(Pressure, ALTITUDE_BASE);
  Altitude = barometer.altitude(Pressure, Pressure_seaLevel);

  tempGauge.setValue(Temperature);
  altBar.setValue(Altitude_relative_filt);
  pressureBar.setValue(Pressure);

  String altText;
  altText += (int) Altitude;
  altText += "m\n(";
  if (Altitude_relative_filt >= 0.0) {
    altText += "+";
  }
  altText += Altitude_relative_filt;
  altText += "m)";
  altValueLabel.setText(altText);

  String pressText;
  pressText += Pressure;
  pressText += " mb";
  pressureValueLabel.setText(pressText);

  display.update();
}