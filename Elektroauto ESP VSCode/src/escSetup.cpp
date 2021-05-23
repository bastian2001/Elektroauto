#include "driver/rmt.h"
#include "global.h"

uint8_t newRedLED, newBlueLED, newGreenLED;

rmt_item32_t escDataBuffer[ESC_BUFFER_ITEMS];

/**
 * @brief initializes the rmt peripheral for DShot data transmittion
 * 
 * @param channel the channel to use
 * @param pin output GPIO pin
 */
void esc_init(uint8_t channel, uint8_t pin) {
  rmt_config_t config;
  config.rmt_mode = RMT_MODE_TX;
  config.channel = ((rmt_channel_t) channel);
  config.gpio_num = ((gpio_num_t) pin);
  config.mem_block_num = 3;
  config.tx_config.loop_en = false;
  config.tx_config.carrier_en = false;
  config.tx_config.idle_output_en = true;
  config.tx_config.idle_level = ((rmt_idle_level_t) 0);
  config.clk_div = 6; // target: DShot 300 (300kbps)

  ESP_ERROR_CHECK(rmt_config(&config));
  ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
}

/**
 * @brief sets up the rmt data buffer
 * 
 * @param value the transmitted DShot-packet
 */
void setup_rmt_data_buffer(uint16_t value) {
  uint16_t mask = 1 << (ESC_BUFFER_ITEMS - 1);
  for (uint8_t bit = 0; bit < ESC_BUFFER_ITEMS; bit++) {
    uint16_t bit_is_set = value & mask;
    escDataBuffer[bit] = bit_is_set ? (rmt_item32_t) {{{T1H, 1, T1L, 0}}} : (rmt_item32_t) {{{T0H, 1, T0L, 0}}};
    mask >>= 1;
  }
}

/**
 * @brief prepares the DShot-packet and sends it
 * 
 * also checks for LED-changes
 * @param value the escValue to transmit, incl. checksum
 * @param wait whether to wait until the transmission is done
 */
void esc_send_value(uint16_t value, bool wait) {
  setup_rmt_data_buffer(value);
  ESP_ERROR_CHECK(rmt_write_items((rmt_channel_t) 0, escDataBuffer, ESC_BUFFER_ITEMS, wait));

  switch(value){
    case 0x0356:
      newRedLED = 1;
      break;
    case 0x02DF:
      newRedLED = 2;
      break;
    case 0x0374:
      newGreenLED = 1;
      break;
    case 0x02FD:
      newGreenLED = 2;
      break;
    case 0x039A:
      newBlueLED = 1;
      break;
    case 0x0312:
      newBlueLED = 2;
      break;
    default:
      break;
  }
}