#include "global.h"

double rawAccelToPhysicalAccel(int raw){ // in mm per s
    return (double) raw / (16.384 / 9.81);
}

void readBMI(){
    rawAccel = -BMI160.getAccelerationX();
    if (telemetryERPM > 0 || throttle > 0){
        acceleration = rawAccelToPhysicalAccel(rawAccel);
        speedBMI += acceleration / (double)ESC_FREQ;
        distBMI += speedBMI / (double)ESC_FREQ;
    } else {
        acceleration = rawAccelToPhysicalAccel(rawAccel);
        speedBMI = 0;
        distBMI = 0;
    }
}

void initBMI(){
    pinMode(26, OUTPUT);
    digitalWrite(26, HIGH);
    delay(20);
    #ifdef PRINT_SETUP
    Serial.println("Connecting to BMI160...");
    #endif
    BMI160.begin(BMI160GenClass::SPI_MODE, 5); //see docs/bmi160 breakout.png and docs/ESP32 Lilygo Pinout.webp for pins


    // checking connection
    bool connected = BMI160.testConnection();

    #ifdef PRINT_SETUP
    if (!connected)
        Serial.println("could not connect to the BMI160 sensor");
    #endif
    
    BMI160.setAccelerometerRange(2); //+/-2g range for high precision
    BMI160.setAccelerometerRate(1600); //1600Hz for fast sampling

    //accelerometer calibration
    BMI160.autoCalibrateXAccelOffset(0);
    BMI160.autoCalibrateYAccelOffset(0);
    BMI160.autoCalibrateZAccelOffset(1);
    BMI160.setAccelOffsetEnabled(true);

    #ifdef PRINT_SETUP
    if (!(BMI160.getRegister(0x1b) & 0x8))
        Serial.println("Couldn't calibrate the BMI160");
    else Serial.println("Calibration finished.");
    #endif

    delay(10);
}