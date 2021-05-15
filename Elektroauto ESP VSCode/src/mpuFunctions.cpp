#include <Arduino.h>
#include "global.h"
// #include "MPU6050.h"
// #include "Wire.h"
// #include "I2Cdev.h"

// //MPU
uint16_t distMPU = 0;
double speedMPU = 0, acceleration = 0;
int16_t raw_accel = 0;
unsigned long lastMPUUpdate = 0;
// MPU6050 mpu;
bool mpuReady = false;
// extern bool updatedValue;
// portMUX_TYPE mpuMux = portMUX_INITIALIZER_UNLOCKED;

// float rawAccelToPhysicalAccel(int acceleration){
//     return (float) acceleration / (16.384f / (float)pow(2, ACCEL_RANGE + 1) / 9.81f);
// }

// void handleMPU(){
//     mpuReady = mpu.testConnection();
//     // Serial.println(mpuReady);
//     if (mpuReady){
//         raw_accel = mpu.getAccelerationX();
//         if (armed && throttle > 0){
//             acceleration = rawAccelToPhysicalAccel(raw_accel);
//             speedMPU += acceleration / 1000.0;
//             distMPU += speedMPU / 1000;
//         } else {
//             acceleration = 0;
//             speedMPU = 0;
//             distMPU = 0;
//         }
//     }
//     updatedValue = true;
// }

// void initMPU(){
//     // start I2C bus (SDA: 18, SCL: 19)
//     Wire.begin(18, 19);
//     Wire.setClock(400000);

//     // initialization of device
//     mpu.initialize();

//     #ifdef PRINT_SETUP
//     if (!mpu.testConnection())
//         Serial.println("could not connect to the MPU6050");
    
//     Serial.println("calibrating accelerometer");
//     #endif

//     //accelerometer calibration
//     mpu.CalibrateAccel();
//     mpu.setFullScaleAccelRange(ACCEL_RANGE);

//     mpuReady = mpu.testConnection();

//     #ifdef PRINT_SETUP
//     Serial.print("Calibration finished. X-offset is ");
//     Serial.print(mpu.getXAccelOffset());
//     Serial.println(" now.");
//     #endif
// }