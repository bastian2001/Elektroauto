//! @file escFunctions.cpp all the neccessary ESC functions

#include "global.h"
#include "system.h"
#include "accelerometerFunctions.h"

rmt_item32_t escDataBuffer[ESC_BUFFER_ITEMS];

int escOutputCounter = 0, escOutputCounter3 = 0;

/**
 * @brief initializes the RMT peripheral for DShot data transmittion
 * 
 * automatically run at startup
 */
void esc_init(rmt_channel_t channel, uint8_t pin) {
  rmt_config_t config;
  config.rmt_mode = RMT_MODE_TX;
  config.channel = channel;
  config.gpio_num = (gpio_num_t) pin;
  config.mem_block_num = 1;
  config.tx_config.loop_en = false;
  config.tx_config.carrier_en = false;
  config.tx_config.idle_output_en = true;
  config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
  config.clk_div = CLK_DIV;

  ESP_ERROR_CHECK(rmt_config(&config));
  ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
}

void IRAM_ATTR setup_rmt_data_buffer(uint16_t value) {
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
 * WARNING: please do not manually run it, escIR runs it at the frequency specified in ESC_FREQ
 * @param value the full packet to transmit
 * @param wait whether to wait until the transmission is done
 */
void IRAM_ATTR esc_send_value(uint16_t value, bool wait, rmt_channel_t channel) {
  setup_rmt_data_buffer(value);
  ESP_ERROR_CHECK(rmt_write_items(channel, escDataBuffer, ESC_BUFFER_ITEMS, wait));

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
void IRAM_ATTR escIR() {
  //set Throttle
  if (armed){
    setThrottle(nextThrottle);
  }
  #if TRANSMISSION_IND != 0
  escOutputCounter = (escOutputCounter == TRANSMISSION_IND - 1) ? 0 : escOutputCounter + 1;
  if (escOutputCounter == 0)
    digitalWrite(TRANSMISSION_PIN, LOW);
  delayMicroseconds(20);
  #endif

  //send value to ESC
  if (manualDataAmount == 0){
    esc_send_value(escValue, false, RMT_CHANNEL_0);
    esc_send_value(escValue, false, RMT_CHANNEL_1);
  }
  else {
    esc_send_value(manualData[0], false, RMT_CHANNEL_0);
    esc_send_value(manualData[0], false, RMT_CHANNEL_1);
    manualDataAmount--;
    for (int i = 0; i < 19; i++){
      manualData[i] = manualData[i+1];
    }
    manualData[19] = 0;
  }
  #if TRANSMISSION_IND != 0
  if (escOutputCounter == 0)
    digitalWrite(TRANSMISSION_PIN, HIGH);
  #endif

  // record new previousERPM value
  for (int i = 0; i < TREND_AMOUNT - 1; i++) {
    previousERPM[i] = previousERPM[i + 1];
  }
  previousERPM[TREND_AMOUNT - 1] = telemetryERPM;

  // handle BMI routine
  readBMI();

  // print debug telemetry over Serial
  #if TELEMETRY_DEBUG != 0
  escOutputCounter3 = (escOutputCounter3 == TELEMETRY_DEBUG - 1) ? 0 : escOutputCounter3 + 1;
  if (escOutputCounter3 == 0){ 
    #ifdef PRINT_TELEMETRY_THROTTLE
      sPrint((String)((int)throttle));
      sPrint("\t");
    #endif
    #ifdef PRINT_TELEMETRY_TEMP
      sPrint((String)telemetryTemp);
      sPrint("\t");
    #endif
    #ifdef PRINT_TELEMETRY_VOLTAGE
      sPrint((String)telemetryVoltage);
      sPrint("\t");
    #endif
    #ifdef PRINT_TELEMETRY_ERPM
      sPrint((String)telemetryERPM);
      sPrint("\t");
    #endif
    #if defined(PRINT_TELEMETRY_THROTTLE) || defined(PRINT_TELEMETRY_TEMP) || defined(PRINT_TELEMETRY_VOLTAGE) || defined(PRINT_TELEMETRY_ERPM)
      sPrintln("");
    #endif
  }
  #endif

  // logging, if race is active
  if (raceActive){
    throttle_log[logPosition] = (uint16_t)throttle;
    acceleration_log[logPosition] = 0;
    erpm_log[logPosition] = previousERPM[TREND_AMOUNT - 1];
    voltage_log[logPosition] = telemetryVoltage;
    temp_log[logPosition] = telemetryTemp;
    logPosition++;
    if (logPosition == LOG_FRAMES){
      raceActive = false;
      raceModeSendValues = true;
    }
  }
}