#include <Arduino.h>

void esc_init(uint8_t channel, uint8_t pin);
void setup_rmt_data_buffer(uint16_t value);
void esc_send_value(uint16_t value, bool wait);