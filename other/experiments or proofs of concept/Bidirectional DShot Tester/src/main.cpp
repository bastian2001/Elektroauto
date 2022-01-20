#include <Arduino.h>
#include "driver/rmt.h"

#define LENGTH 6

volatile int intr = 0;
volatile bool on = false;
rmt_isr_handle_t xHandler = NULL;
uint32_t lastMicros = 0;
volatile rmt_item32_t items[LENGTH][64];
int length[LENGTH];
int ms[LENGTH];

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
	intr++;
	if (intr < LENGTH + 2){
		uint32_t m = micros();
		if (intr > 1){
			ms[intr - 2] = m - lastMicros;
			length[intr - 2] = rmt_get_mem_len(RMT_CHANNEL_0);
			volatile rmt_item32_t* data = RMTMEM.chan[RMT_CHANNEL_0].data32;
			for ( int i = 0; i < 64; i++){
				if (data[i].duration0 != 0){
					items[intr - 2][i].duration0 = data[i].duration0;
					items[intr - 2][i].level0 = data[i].level0;
					items[intr - 2][i].duration1 = data[i].duration1;
					items[intr - 2][i].level1 = data[i].level1;
				}
			}
		}
		lastMicros = m;
	}
	uint32_t intr_st = RMT.int_st.val;

	auto& conf = RMT.conf_ch[0].conf1;
	conf.rx_en = 0;
	conf.mem_owner = RMT_MEM_OWNER_TX;
	// volatile rmt_item32_t* item = RMTMEM.chan[0].data32;
	// if (item && on) {
	// 	Serial.printf("%d %d %d %d\n", item->duration0, item->level0 * 20, item->duration1, item->level1 * 20);
	// }

	conf.mem_wr_rst = 1;
	conf.mem_owner = RMT_MEM_OWNER_RX;
	conf.rx_en = 1;
	RMT.int_clr.val = intr_st;
}

void telemetryInit(rmt_channel_t channel, gpio_num_t pin){
	rmt_config_t c;
	c.rmt_mode = RMT_MODE_RX;
	c.channel = channel;
	c.gpio_num = pin;
	c.mem_block_num = 1;
	c.clk_div = 6;
	c.rx_config.idle_threshold = 300;
	ESP_ERROR_CHECK(rmt_config(&c));
	ESP_ERROR_CHECK(rmt_set_rx_intr_en(channel, true));
	ESP_ERROR_CHECK(rmt_set_tx_intr_en(channel, false));
	ESP_ERROR_CHECK(rmt_set_tx_thr_intr_en(channel, false, 20));
	ESP_ERROR_CHECK(rmt_set_err_intr_en(channel, false));
	ESP_ERROR_CHECK(rmt_isr_register(isr, NULL, ESP_INTR_FLAG_LEVEL1, &xHandler));
	rmt_rx_start(RMT_CHANNEL_0, true);
}


void setup() {
	// put your setup code here, to run once:
	Serial.begin(230400);
	telemetryInit(RMT_CHANNEL_0, GPIO_NUM_13);
}

uint32_t counter = 0;
void loop() {
	// put your main code here, to run repeatedly:
	// if (counter++ % 1000000 == 0){
	// 	// Serial.println(counter/10000);
	// 	// Serial.println(intr);
	// }
	// on = !digitalRead(5);
	counter += 1000;
	while (millis() < counter){}
	Serial.println('\n');
	for (int i = 0; i < LENGTH; i++){
		Serial.println(ms[i]);
		Serial.println(length[i]);
		for (int j = 0; j < length[i]; j++){
			Serial.printf("%d %d\t", items[i][j].duration0, items[i][j].duration1);
		}
		Serial.println();
	}
	Serial.println(intr);
	intr = 0;
}
