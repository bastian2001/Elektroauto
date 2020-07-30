#include "driver/rmt.h"
#include "global.h"

rmt_item32_t escDataBuffer[ESC_BUFFER_ITEMS];

void esc_init(uint8_t channel, uint8_t pin) {
  rmt_config_t config;
  config.rmt_mode = RMT_MODE_TX;
  config.channel = (rmt_channel_t) channel;
  config.gpio_num = (gpio_num_t) pin;
  config.mem_block_num = 3;
  config.tx_config.loop_en = false;
  config.tx_config.carrier_en = false;
  config.tx_config.idle_output_en = true;
  config.tx_config.idle_level = (rmt_idle_level_t) 0;
  config.clk_div = 6; // target: DShot 300 (300kbps)

  ESP_ERROR_CHECK(rmt_config(&config));
  ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
}

void setup_rmt_data_buffer(uint16_t value) {
  uint16_t mask = 1 << (ESC_BUFFER_ITEMS - 1);
  for (uint8_t bit = 0; bit < ESC_BUFFER_ITEMS; bit++) {
    uint16_t bit_is_set = value & mask;
    escDataBuffer[bit] = bit_is_set ? (rmt_item32_t) {{{T1H, 1, T1L, 0}}} : (rmt_item32_t) {{{T0H, 1, T0L, 0}}};
    mask >>= 1;
  }
}

void esc_send_value(uint16_t value, bool wait) {
  setup_rmt_data_buffer(value);
  ESP_ERROR_CHECK(rmt_write_items((rmt_channel_t) 0, escDataBuffer, ESC_BUFFER_ITEMS, wait));
}