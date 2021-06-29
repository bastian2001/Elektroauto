#include <Arduino.h>
#include "global.h"
#include "BMI160Gen.h"

double rawAccelToPhysicalAccel(int raw){ // in mm per s
    return (double) raw / (16.384 / 9.81);
}

void readBMI(){
    rawAccel = BMI160.getAccelerationX();
    // if (armed && throttle > 0){
        acceleration = rawAccelToPhysicalAccel(rawAccel);
        speedBMI += acceleration / 1000.0;
        distBMI += speedBMI / 1000;
    // } else {
    //     speedBMI = 0;
    //     distBMI = 0;
    // }
}

void initBMI(){
    BMI160.begin(BMI160GenClass::SPI_MODE, 5); //see docs/bmi160 breakout.png and docs/ESP32 Lilygo Pinout.webp for pins


    // checking connection
    uint8_t devID = BMI160.getDeviceID();

    #ifdef PRINT_SETUP
    if (devID != 0xd1)
        Serial.println("could not connect to the BMI160 sensor");
    
    #endif

    BMI160.setAccelerometerRange(2); //+/-2g range for high precision
    BMI160.setAccelerometerRate(1600); //1600Hz for fast sampling


    //accelerometer calibration
    BMI160.setRegister(0x7e, 0x03); //command register, command: do fast offset compensation
    for (uint8_t i = 0; i < 300; i++){
        if (BMI160.getRegister(0x1b) & 0x8)
            break;
        delay(1);
    }

    #ifdef PRINT_SETUP
    if (!(BMI160.getRegister(0x1b) & 0x8))
        Serial.println("Couldn't calibrate the BMI160");
    else Serial.println("Calibration finished.");
    #endif
}