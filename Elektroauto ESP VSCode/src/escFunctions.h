//! @file escFunctions.h all the neccessary ESC functions

#include <Arduino.h>

// /**
//  * @brief initializes the RMT peripheral for DShot data transmittion
//  * 
//  * automatically run at startup
//  * @param channel the channel to be initialized
//  * @param pin the pin number
//  */
// void esc_init(rmt_channel_t channel, uint8_t pin);

// /**
//  * @brief prepares the DShot-packet and sends it
//  * 
//  * also checks for LED changes
//  * 
//  * WARNING: please do not manually run it, escIR runs it at the frequency specified in ESC_FREQ
//  * @param value the full packet to transmit
//  * @param wait whether to wait until the transmission is done
//  * @param channel the channel where the value should be sent
//  */
// void IRAM_ATTR esc_send_value(uint16_t value, bool wait, rmt_channel_t channel);

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