/*======================================================includes======================================================*/

#include "driver/rmt.h"



/*======================================================definitions======================================================*/

//Pin numbers
#define ESC_OUTPUT_PIN  25  // the signal pin
#define ESC_TRIGGER_PIN 32  // pwm pin for output trigger
#define TRANSMISSION  33    //transmission indicator
#define LED_BUILTIN 22

//ESC values
#define ESC_FREQ 4000
#define ESC_BUFFER_ITEMS 16
#define CLK_DIV 2; // DShot 300: 4, DShot 600: 2, DShot 1200: 1
#define TT 67 // total bit time
#define T0H 25 // 0 bit high time
#define T1H 50 // 1 bit high time
#define T0L (TT - T0H) // 0 bit low time
#define T1L (TT - T1H) // 1 bit low time
#define TRANSMISSION_IND ESC_FREQ * 1.5


/*======================================================global variables======================================================*/

//Core 0 variables
TaskHandle_t Task1;
String toBePrinted = "";

//system variables
double throttle = 0;
bool c1ready = false;
uint16_t escValue = 0xf;

//ESC variables
rmt_item32_t esc_data_buffer[ESC_BUFFER_ITEMS];

//arbitrary variables
int16_t escOutputCounter = 0;


/*======================================================system methods======================================================*/

void setup() {

  //Serial setup
  Serial.begin(115200);
  disableCore0WDT();
  disableCore1WDT();

  //ESC pins Setup
  pinMode(ESC_OUTPUT_PIN, OUTPUT);
  pinMode(ESC_TRIGGER_PIN, OUTPUT);
  pinMode(TRANSMISSION, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(TRANSMISSION, HIGH);
  esc_init(0, ESC_OUTPUT_PIN);
  ledcSetup(1, ESC_FREQ, 8);
  ledcAttachPin(ESC_TRIGGER_PIN, 1);
  ledcWrite(1, 127);

  //2nd core setup
  xTaskCreatePinnedToCore( Task1code, "Task1", 10000, NULL, 0, &Task1, 0);

  //setup termination
  Serial.println("ready");
  c1ready = true;
}

void Task1code( void * parameter) {
  disableCore0WDT();
  attachInterrupt(digitalPinToInterrupt(ESC_TRIGGER_PIN), escir, RISING);

  while (!c1ready) {
    yield();
  }

  while (true) {
  }
}

uint16_t val;
void loop() {
  if (Serial.available()) {
    String readout = Serial.readStringUntil('\n');
    val = readout.toInt();
    val = (val > 500) ? 500 : val;
    escValue = appendChecksum(val);
  }
}


/*======================================================functional methods======================================================*/

void escir() {
  rmt_set_idle_level((rmt_channel_t) 0, true, (rmt_idle_level_t) 1);
  escOutputCounter = (escOutputCounter == TRANSMISSION_IND) ? 0 : escOutputCounter + 1;
  digitalWrite(TRANSMISSION, HIGH);
  if (escOutputCounter == 0)
    digitalWrite(TRANSMISSION, LOW);
  delayMicroseconds(6);
  esc_send_value(escValue, false);
  delayMicroseconds(25);
  rmt_set_idle_level((rmt_channel_t) 0, false, (rmt_idle_level_t) 1);
}

uint16_t appendChecksum(uint16_t value) {
  value &= 0x7FF;
  value = (value << 1) | 0;
  int csum = 0, csum_data = value;
  for (int i = 0; i < 3; i++) {
    csum ^=  csum_data;   // xor data by nibbles
    csum_data >>= 4;
  }
  csum = ~csum;
  csum &= 0xf;
  value = (value << 4) | csum;
  return value;
}

void esc_init(uint8_t channel, uint8_t pin) {
  rmt_config_t config;
  config.rmt_mode = RMT_MODE_TX;
  config.channel = ((rmt_channel_t) channel);
  config.gpio_num = ((gpio_num_t) pin);
  config.mem_block_num = 1;
  config.tx_config.loop_en = false;
  config.tx_config.carrier_en = false;
  config.tx_config.idle_output_en = true;
  config.tx_config.idle_level = ((rmt_idle_level_t) 1);
  config.clk_div = CLK_DIV; // target: DShot 300 (300kbps)

  ESP_ERROR_CHECK(rmt_config(&config));
  ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
}

void esc_send_value(uint16_t value, bool wait) {
  setup_rmt_data_buffer(value);
  ESP_ERROR_CHECK(rmt_write_items((rmt_channel_t) 0, esc_data_buffer, ESC_BUFFER_ITEMS, wait));
}

void setup_rmt_data_buffer(uint16_t value) {
  uint16_t mask = 1 << (ESC_BUFFER_ITEMS - 1);
  for (uint8_t bit = 0; bit < ESC_BUFFER_ITEMS; bit++) {
    uint16_t bit_is_set = value & mask;
    esc_data_buffer[bit] = bit_is_set ? (rmt_item32_t) {
      {{
          T1H, 0, T1L, 1
        }
      }
} : (rmt_item32_t) {
      {{
          T0H, 0, T0L, 1
        }
      }
    };
    mask >>= 1;
  }
}
