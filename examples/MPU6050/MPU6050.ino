/*
 * Shows live results from the MPU6050 Accelerometer/Gyro
 */
#include "Phoenard.h"
#include <Wire.h>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"

// Define the MPU6050 sensor using the library
MPU6050 accelgyro(MPU6050_ADDRESS_AD0_LOW);

uint16_t areaGyro[3] = {18, (18 + 118) / 2, 118};
uint16_t areaAccelerometer[3] = {132, (132 + 239) / 2, 239};

uint32_t accelNow[3];
uint32_t accelPast[3]={areaAccelerometer[1],areaAccelerometer[1],areaAccelerometer[1]};
uint32_t gyroNow[3];
uint32_t gyroPast[3]={areaGyro[1],areaGyro[1],areaGyro[1]};
int16_t accel[3];
int16_t gyro[3];
  
uint16_t iteration=300;

// Define the colors to use for X/Y/Z coordinates
uint16_t color[3]={BLUE,GREEN,YELLOW};

void setup() {
  // Initialize serial communication
  Serial.begin(9600);

  // Initialize the OneWire library
  Serial.println("Initializing OneWire");
  Wire.begin();
  
  // Initialize the MPU6050 sensor and show whether it worked
  Serial.println("Initializing MPU6050");
  accelgyro.initialize();
  if (accelgyro.testConnection()) {
    Serial.println("MPU6050 connection successful");
  } else {
    Serial.println("MPU6050 connection failed");
  }
  
  // Draw the frame lines
  display.drawLine(0,areaGyro[0] - 1, display.width(), areaGyro[0] - 1, RED);
  display.drawLine(0,areaGyro[2] + 1, display.width(), areaGyro[2] + 1, RED);
  display.drawLine(0,areaAccelerometer[0] - 1, display.width(), areaAccelerometer[0] - 1, RED);
  
  // Draw text labels
  display.setTextColor(WHITE);
  display.setTextSize(1);
  
  display.setCursor((display.width()/2)-(5*15), areaGyro[0] - 10);
  display.print("Angular Frequency Test (Gyro)");
  
  display.setCursor((display.width()/2)-(5*9), areaAccelerometer[0] - 10);
  display.print("Acceleration test");
}

void loop() {
  // Read raw acceleration and gyro rotation information
  accelgyro.getAcceleration(&accel[0], &accel[1], &accel[2]);
  accelgyro.getRotation(&gyro[0], &gyro[1], &gyro[2]);

  // Draw each x/y/z coordinate lines
  for (int i=0;i<3;i++) {
    // Remap the accelerometer data to the area to display in
    // Then connect the previous result point to the next with a line
    // Finally set the previous value to the current new value
    accelNow[i]= map(accel[i], -20000, 20000, areaAccelerometer[0], areaAccelerometer[2]);
    accelNow[i] = constrain(accelNow[i], areaAccelerometer[0], areaAccelerometer[2]);
    display.drawLine(iteration,accelPast[i],iteration+1,accelNow[i],color[i]);      
    accelPast[i]=accelNow[i];

    // Remap the gyrometer data to the area to display in
    // Then connect the previous result point to the next with a line
    // Finally set the previous value to the current new value
    gyroNow[i] = map(gyro[i], -40000, 40000, areaGyro[0], areaGyro[2]);
    gyroNow[i] = constrain(gyroNow[i], areaGyro[0], areaGyro[2]);
    display.drawLine(iteration,gyroPast[i],iteration+1,gyroNow[i],color[i]);  
    gyroPast[i]=gyroNow[i];  
  }

  iteration++;
  if (iteration==(display.width()-2)){
    iteration=1;

    // Wipe the graph
    display.fillRect(1, areaAccelerometer[0], display.width()-2, areaAccelerometer[2] - areaAccelerometer[0] + 1, BLACK);        
    display.fillRect(1, areaGyro[0], display.width()-2, areaGyro[2] - areaGyro[0] + 1, BLACK);

    // Log the data to Serial now and then
    Serial.print("Accel: [");
    for (int i = 0; i < 3; i++) {
      if (i > 0) Serial.print("  ");
      Serial.print(accel[i]);
    }
    Serial.print("]  |  ");
    Serial.print("Gyro: [");
    for (int i = 0; i < 3; i++) {
      if (i > 0) Serial.print("  ");
      Serial.print(gyro[i]);
    }
    Serial.println("]");
  }
    
  delay(1);
}
