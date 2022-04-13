#include <Arduino.h>
#include "driver/rmt.h"

volatile int intrCounter;

rmt_isr_handle_t xHandler = NULL;
volatile rmt_item32_t itemBuf[64];

int itemLength = 0;

bool newData = false;

uint8_t data[500];
uint8_t preData[30000];
uint16_t dataLength = 0;


#define BB_INVALID 0xffffffff

IRAM_ATTR void bufToBin(volatile rmt_item32_t items[64], int startPos, int length){
    Serial.println("trig2");
  for (int i = startPos; i < length; i++){
    for (int j = 0; j < items[i].duration0; j++){
      if (itemLength < 30000){
        preData[itemLength++] = items[i].level0;
      } else return;
    }
    for (int j = 0; j < items[i].duration1; j++){
      if (itemLength < 30000){
        preData[itemLength++] = items[i].level1;
      } else return;
    }
  }
  for (int i = 0; i < itemLength / 60; i++){
    data[i] = preData[60*i];
  }
  itemLength /= 60;
}

int IRAM_ATTR rmt_get_mem_len(rmt_channel_t channel)
{
    int block_num = RMT.conf_ch[channel].conf0.mem_size;
    int item_block_len = block_num * RMT_MEM_ITEM_NUM;
    volatile rmt_item32_t* data = RMTMEM.chan[channel].data32;
    int idx;
    for(idx = 0; idx < item_block_len; idx++) {
        if(data[idx].duration0 == 0) {
            return idx;
        } else if(data[idx].duration1 == 0) {
            return idx + 1;
        }
    }
    return idx;
}

void IRAM_ATTR isr(void *arg){
    Serial.println("trig");
  
	itemLength = rmt_get_mem_len(RMT_CHANNEL_0);
	volatile rmt_item32_t* data = RMTMEM.chan[RMT_CHANNEL_0].data32;
	for ( int i = 0; i < 64; i++){
		if (data[i].duration0 != 0){
			((uint32_t *)itemBuf)[i] = ((uint32_t *)data)[i];
		}
	}

  bufToBin(itemBuf, 0, itemLength);

	uint32_t intr_st = RMT.int_st.val;

	auto& conf = RMT.conf_ch[0].conf1;
	conf.rx_en = 0;
	conf.mem_owner = RMT_MEM_OWNER_TX;

	conf.mem_wr_rst = 1;
	conf.mem_owner = RMT_MEM_OWNER_RX;
	RMT.int_clr.val = intr_st;
	conf.rx_en = 1;

  newData = true;
}

void pwmInit(rmt_channel_t channel, gpio_num_t pin){
	rmt_config_t c;
	c.rmt_mode = RMT_MODE_RX;
	c.channel = channel;
	c.gpio_num = pin;
	c.mem_block_num = 1;
	c.clk_div = 240;
	c.rx_config.idle_threshold = 1500;
	c.rx_config.filter_en = false;
	ESP_ERROR_CHECK(rmt_config(&c));
	ESP_ERROR_CHECK(rmt_set_rx_intr_en(channel, true));
	ESP_ERROR_CHECK(rmt_set_tx_intr_en(channel, false));
	ESP_ERROR_CHECK(rmt_set_tx_thr_intr_en(channel, false, 30));
	ESP_ERROR_CHECK(rmt_set_err_intr_en(channel, false));
	ESP_ERROR_CHECK(rmt_isr_register(&isr, NULL, ESP_INTR_FLAG_LEVEL1, &xHandler));
	rmt_rx_start(channel, true);
}


void setup() {
	// put your setup code here, to run once:
	Serial.begin(230400);
	pwmInit(RMT_CHANNEL_0, GPIO_NUM_13);

  // pinMode(13, INPUT);
}


void loop() {
  if (newData){
    newData = false;
    Serial.println("data!!!!!");
    int start = (500 - itemLength) / 2;
    for (int i = 0; i < start; i++){
      Serial.println(0);
    }
    for (int i = 0; i < itemLength; i++){
      Serial.println(data[i]);
    }
    for (int i = 0; i < start; i++){
      Serial.println(0);
    }
  }

  // Serial.println(digitalRead(13));
}
