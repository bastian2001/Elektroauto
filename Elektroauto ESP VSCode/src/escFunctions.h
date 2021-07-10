//! @file escFunctions.h all the neccessary ESC functions

#include <Arduino.h>

/**
 * @brief initializes the RMT peripheral for DShot data transmittion
 * 
 * automatically run at startup
 */
void esc_init(rmt_channel_t channel, uint8_t pin);

/**
 * @brief prepares the DShot-packet and sends it
 * 
 * WARNING: please do not manually run it, escIR runs it at the frequency specified in ESC_FREQ
 * @param value the full packet to transmit
 * @param wait whether to wait until the transmission is done
 */
void esc_send_value(uint16_t value, bool wait, rmt_channel_t channel);

/**
 * @brief the ESC interrupt routine
 * 
 * WARNING: please do not manually run it, the timer interrupt runs it at the frequency specified in ESC_FREQ
 */
void escIR();