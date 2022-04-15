#include "global.h"

double rawAccelToPhysicalAccel(int raw){ // in mm per s
    return (double) raw / (16.384 / 9.81) * 4;
}

void readBMI(){
    rawAccel = -BMI160.getAccelerationX();
    if (ESCs[0]->eRPM > 0 || ESCs[0]->currentThrottle > 0){
        acceleration = rawAccelToPhysicalAccel(rawAccel);
        speedBMI += acceleration / (double)ESC_FREQ;
        distBMI += speedBMI / (double)ESC_FREQ;
    } else {
        acceleration = rawAccelToPhysicalAccel(rawAccel);
        speedBMI = 0;
        distBMI = 0;
    }
    bmiRawTemp = BMI160.getTemperature();
    bmiTemp = 23 + bmiRawTemp / 512;
}

void calibrateAccelerometer(){
}

void initBMI(){
    #ifdef PRINT_SETUP
    Serial.println("Connecting to BMI160...");
    #endif
    BMI160.begin(BMI160GenClass::SPI_MODE, 5); //see docs/bmi160 breakout.png and docs/ESP32 Lilygo Pinout.webp for pins


    // checking connection
    bool connected = BMI160.testConnection();

    if (!connected){
        #ifdef PRINT_SETUP
            Serial.println("could not connect to the BMI160 sensor");
        #endif
        return;
    }
    
    BMI160.setAccelerometerRange(8); //+/-2g range for high precision
    BMI160.setAccelerometerRate(1600); //800Hz for no aliasing (is default anyway)

    
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