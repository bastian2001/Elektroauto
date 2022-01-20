/// @file system.h provides functions for general usability

#include <Arduino.h>

/**
 * @brief arms or disarms the car
 * 
 * always use this function as it also sets the throttle to a safe 0 if the race is inactive
 * @param arm whether to arm or disarm the car
 */
void setArmed(bool arm);

/**
 * @brief sets the mode of the car
 * 
 * @param mode any MODE_*
 */
void setMode(uint8_t mode);

/**
 * @brief Sets the ESC value with the appended checksum
 * 
 * WARNING: There are no further armed checks between this function and the sending of the value, alter the "nextThrottle" variable instead. The offset for the settings is automatically applied
 * @param newThrottle the new throttle between 0 and 2000, value is floored
 */
void IRAM_ATTR setThrottle(double newThrottle);

//! @brief starts the race
void startRace();

/**
 * @brief calculates the next throttle value in speed control mode
 * 
 * @param target the target hERPM value
 * @param was array of previous hERPM value
 * @param currentThrottle the current Throttle
 * @param masterMultiplier default: 1, another GP multiplier for quick adjustments
 * @return the next throttle value (NOT set automatically)
 */
double calcThrottle(int target, int was[], double currentThrottle, double masterMultiplier = 1);

//! @brief reads the Serial0 port and processes the message using processMessage
void receiveSerial();

//! @brief processes Action objects
void runActions();

/**
 * @brief converts RPS to hERPM
 * 
 * @param rps the speed in RPS
 * @return the speed in hERPM
 */
float rpsToErpm(float rps);

/**
 * @brief converts hERPM to RPS
 * 
 * @param erpm the speed in hERPM
 * @return the speed in RPS
 */
float erpmToRps(float erpm);

/**
 * @brief Set the new target value based on reqValue
 * 
 * This is not the throttle routine. It merely sets the target value based on the current mode and reqValue
 */
void setNewTargetValue();

//! @brief sends the race log to all connected phones
void sendRaceLog();

/**
 * @brief throttle routine
 * 
 * calculates the nextThrottle if neccessary (if in mode 1 or 2), automatically executed as often as possible
 * the variable throttle is not mutated, thus the frequency this method is eqecuted is almost irrelevant
 */
void throttleRoutine();

//! @brief checks every 10s if the voltage is too low, initiates warning via broadcast if neccessary
void checkVoltage();

/**
 * @brief gets the current maximum requestable value
 * @param mode the mode to check for
 * @return the maximum value
 */
uint16_t getMaxValue (int mode);