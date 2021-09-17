/*
 * Copyright (c) 2016 Intel Corporation.  All rights reserved.
 * See the bottom of this file for the license terms.
 */

/*
   This sketch example demonstrates how the BMI160 on the
   Intel(R) Curie(TM) module can be used to read gyroscope data
*/
      
#include <BMI160Gen.h>
unsigned long lastMicros = 0;
  int axRaw[500], ayRaw[500], azRaw[500];

void setup() {
  Serial.begin(115200); // initialize Serial communication
  while (!Serial);    // wait for the serial port to open

  // initialize device
  Serial.println("Initializing IMU device...");
  BMI160.begin(BMI160GenClass::SPI_MODE, /* SS pin# = */5);
  uint8_t dev_id = BMI160.getDeviceID();
  Serial.print("DEVICE ID: ");
  Serial.println(dev_id, HEX);

  BMI160.setAccelerometerRange(2);
  BMI160.setAccelerometerRate(1600);
  Serial.println("Initializing IMU device...done.");
  delay(100);
}

float rawToAcc(int rawValue){
  return (float)rawValue / 32768 * 2;
}

void loop() {
  unsigned long start = micros();
  
  for (int i = 0; i < 500; i++){
    BMI160.readAccelerometer(axRaw[i], ayRaw[i], azRaw[i]);
  }
  int delta = micros() - start;

  for (int i = 0; i < 500; i++){
    Serial.print("g:\t");
    Serial.printf("%4.3f", rawToAcc(axRaw[i]));
    Serial.print("\t");
    Serial.printf("%4.3f", rawToAcc(ayRaw[i]));
    Serial.print("\t");
    Serial.printf("%4.3f", rawToAcc(azRaw[i]));
    Serial.println();
  }
  Serial.println(delta);
  Serial.println(String("Read with (Hz): ") + String(500.0/(float) delta*1000000));
  Serial.println(String("One reading took (µs): ") + String((float) delta/500));
  delay(10000);
}