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
 * @brief Sets the ESC value with the appended checksum
 * 
 * WARNING: There are no further armed checks between this function and the sending of the value, alter the "nextThrottle" variable instead. The offset for the settings is automatically applied
 * @param newThrottle the new throttle between 0 and 2000, value is floored
 */
void setThrottle(double newThrottle);

//! @brief starts the race
void startRace();

//! @brief reads the Serial0 port and processes the message using processMessage
void receiveSerial();

/**
 * @brief Set the new target value based on reqValue
 * 
 * This is not the throttle routine. It merely sets the target value based on the current mode and reqValue
 */
void setNewTargetValue();

/**
 * @brief gets the current maximum requestable value
 * @param mode the mode to check for
 * @return the maximum value
 */
uint16_t getMaxValue ();

/**
 * @brief enables or disables the race mode (not the race itself)
 * 
 * @param en whether to enable or disable
 */
void enableRaceMode(bool en);