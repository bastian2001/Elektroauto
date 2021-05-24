#include <Arduino.h>

/**
 * @brief initializes the rmt peripheral for DShot data transmittion
 * 
 * @param channel the channel to use
 * @param pin output GPIO pin
 */
void esc_init(uint8_t channel, uint8_t pin);

/**
 * @brief sets up the rmt data buffer
 * 
 * @param value the transmitted DShot-packet
 */
void setup_rmt_data_buffer(uint16_t value);

/**
 * @brief prepares the DShot-packet and sends it
 * 
 * also checks for LED-changes
 * @param value the escValue to transmit, incl. checksum
 * @param wait whether to wait until the transmission is done
 */void esc_send_value(uint16_t value, bool wait);