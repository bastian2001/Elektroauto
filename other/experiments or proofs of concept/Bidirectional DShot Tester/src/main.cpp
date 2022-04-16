#include <Arduino.h>
#include "driver/rmt.h"

#define AVGCOUNT 39
#define MOTOR_POLES 12

volatile int intrCounter;
uint32_t millisCounter;

uint32_t lastMicros;
int currentDuration;

rmt_isr_handle_t xHandler = NULL;
volatile rmt_item32_t itemBuf[64];
int itemLength;

volatile uint32_t rpsValues[AVGCOUNT];
volatile int rpspos;
volatile uint32_t pRpsValue;


uint32_t csum;
uint32_t value = 0x1fffff;
uint32_t decodedValue = 0;

uint32_t avg;

#define BB_INVALID 0xffffffff

IRAM_ATTR uint32_t bufToRps(volatile rmt_item32_t items[64], int startPos, int length){
	int pos = 20;
	value = 0x1fffff;
	//read 21 bit double encoded value from rmt buffer
	// for (int i = 16; i < length; i++){
	// 	int dur0 = items[i].duration0 + 15;
	// 	int dur1 = items[i].duration1 + 15;
	// 	dur0 /= 105;
	// 	dur1 /= 105;
	// 	for (int i = 0; i < dur0; i++){
	// 		if (pos < 0){
	// 			return 500;
	// 		}
	// 		value ^= 1 << pos--;
	// 	}
	// 	pos -= dur1;
	// }

	//perform 21 bit -> 20 bit decoding
	value = (value >> 1) ^ value;

	#define iv 0xffffffff
    value &= 0xfffff;
    static const uint32_t decode[32] = {
        iv, iv, iv, iv, iv, iv, iv, iv, iv, 9, 10, 11, iv, 13, 14, 15,
        iv, iv, 2, 3, iv, 5, 6, 7, iv, 0, 8, 1, iv, 4, 12, iv };

	//perform GCR decoding
    decodedValue = decode[value & 0x1f];
    decodedValue |= decode[(value >> 5) & 0x1f] << 4;
    decodedValue |= decode[(value >> 10) & 0x1f] << 8;
    decodedValue |= decode[(value >> 15) & 0x1f] << 12;
	
	//calculate checksum
    csum = decodedValue;
    csum = csum ^ (csum >> 8);
    csum = csum ^ (csum >> 4);

    if ((csum & 0xf) != 0xf || decodedValue > 0xffff) {
		intrCounter++;
		return 1;

    } else {
        value = decodedValue >> 4;
        if (value == 0x0fff) {
            return 0;
        }
        value = (value & 0x000001ff) << ((value >> 9) & 0x7);
        if (!value) {
            return 1500;
        }
        value = (1000000 * 60 + value * 50) / value;
    }
	return value / (MOTOR_POLES / 2)/60;
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
	uint32_t m = micros();
	currentDuration = m - lastMicros;
	itemLength = rmt_get_mem_len(RMT_CHANNEL_0);
	volatile rmt_item32_t* data = RMTMEM.chan[RMT_CHANNEL_0].data32;
	for ( int i = 0; i < 64; i++){
		if (data[i].duration0 != 0){
			((uint32_t *)itemBuf)[i] = ((uint32_t *)data)[i];
		}
	}
	lastMicros = m;
	uint32_t intr_st = RMT.int_st.val;

	auto& conf = RMT.conf_ch[0].conf1;
	conf.rx_en = 0;
	conf.mem_owner = RMT_MEM_OWNER_TX;

	conf.mem_wr_rst = 1;
	conf.mem_owner = RMT_MEM_OWNER_RX;
	RMT.int_clr.val = intr_st;
	conf.rx_en = 1;

	if (rpspos == AVGCOUNT) rpspos-=3;
	uint32_t value = bufToRps((volatile rmt_item32_t*)itemBuf, 16, itemLength);
	rpsValues[rpspos++] = value;
	// Serial.println(value);
	// intr++;
	// Serial.print(' ');
	// Serial.println(ms);
	pRpsValue = value;
}

void telemetryInit(rmt_channel_t channel, gpio_num_t pin){
	rmt_config_t c;
	c.rmt_mode = RMT_MODE_RX;
	c.channel = channel;
	c.gpio_num = pin;
	c.mem_block_num = 1;
	c.clk_div = 1;
	c.rx_config.idle_threshold = 2700;
	c.rx_config.filter_en = true;
	c.rx_config.filter_ticks_thresh = 5;
	ESP_ERROR_CHECK(rmt_config(&c));
	ESP_ERROR_CHECK(rmt_set_rx_intr_en(channel, true));
	ESP_ERROR_CHECK(rmt_set_tx_intr_en(channel, false));
	ESP_ERROR_CHECK(rmt_set_tx_thr_intr_en(channel, false, 30));
	ESP_ERROR_CHECK(rmt_set_err_intr_en(channel, false));
	ESP_ERROR_CHECK(rmt_isr_register(isr, (void*)15, ESP_INTR_FLAG_LEVEL1, &xHandler));
	rmt_rx_start(channel, true);
}


void setup() {
	// put your setup code here, to run once:
	Serial.begin(230400);
	telemetryInit(RMT_CHANNEL_0, GPIO_NUM_13);
}
void loop() {
	// put your main code here, to run repeatedly:
	millisCounter += 10;
	while (millis() < millisCounter){}
	
	uint32_t avg = 0;
	uint32_t highest = 0;
	for (int i = 0; i < AVGCOUNT; i++){
		avg += rpsValues[i];
		if (rpsValues[i] > highest) highest = rpsValues[i];
		// Serial.println(rpsValues[i]);
	}
	avg -= highest;
	avg/=AVGCOUNT-1;
	Serial.println(avg);
	// Serial.println(rpsValues[0] == rpsValues[50] ? "true" : "false");
	// Serial.println(intr);
	
	rpspos = 0;
	// Serial.println(ms);
	intrCounter = 0;
}
