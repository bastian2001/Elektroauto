//! @file escFunctions.h all the neccessary ESC functions

#include <Arduino.h>

/**
 * @brief the ESC interrupt routine
 * 
 * WARNING: please do not manually run it, the timer interrupt runs it at the frequency specified in ESC_FREQ
 * - runs setThrottle if car is armed, thus applies the checksum
 * - if available, it takes the manualData inplace of the escValue and sends the respective value to the esc
 * - then, if neccessary, it pushes the manualData array one to the front
 * - same thing goes for telemetryERPM history
 * - if the race is active, it also logs the data
 */
void escIR();