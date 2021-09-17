/// @file accelerometerFunctions.h provides functions to read/set up the accelerometer

/**
 * @brief reads data from the BMI160 sensor
 * 
 * - calculates acceleration
 * - calculates the speed
 * - calculates the traveled distance
 */
void readBMI();

/**
 * @brief initializes the BMI160 sensor
 * 
 * connects to the sensor, sets the polling rate and calibrates it
 */
void initBMI();

/**
 * @brief calibrates the accelerometer
 * 
 * enables offset and calculates it
 */
void calibrateAccelerometer();