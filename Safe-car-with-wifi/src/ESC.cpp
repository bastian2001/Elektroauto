#include "Arduino.h"
#include "driver/rmt.h"
#include "ESC.h"

uint8_t get_crc8(uint8_t *Buf, uint8_t BufLen);

uint16_t ESC::maxThrottle = 2000;

rmt_isr_handle_t xHandler = NULL;
volatile rmt_item32_t itemBuf[64];


ESC::ESC(int8_t signalPin, uint8_t telemetryPin, rmt_channel_t dmaChannelTX, rmt_channel_t dmaChannelRX)
: dmaChannelTX(dmaChannelTX),
dmaChannelRX(dmaChannelRX),
signalPin(signalPin),
telemetryPin(telemetryPin)
{

    pinMode (signalPin, OUTPUT);
    rmt_config_t c;
    c.rmt_mode = RMT_MODE_TX;
    c.channel = dmaChannelTX;
    c.clk_div = CLK_DIV;
    c.gpio_num = (gpio_num_t) signalPin;
    c.mem_block_num = 1;
    c.tx_config.loop_en = false;
    c.tx_config.carrier_en = false;
    c.tx_config.idle_level = RMT_IDLE_LEVEL_HIGH;
    c.tx_config.idle_output_en = true;
    ESP_ERROR_CHECK(rmt_config(&c));
    ESP_ERROR_CHECK(rmt_driver_install(dmaChannelTX, 0, 0));

	rmt_config_t d;
	d.rmt_mode = RMT_MODE_RX;
	d.channel = dmaChannelRX;
	d.gpio_num = (gpio_num_t)telemetryPin;
	d.mem_block_num = 1;
	d.clk_div = CLK_DIV;
	d.rx_config.idle_threshold = 2700;
	d.rx_config.filter_en = true;
	d.rx_config.filter_ticks_thresh = 5;
	ESP_ERROR_CHECK(rmt_config(&d));
    ESP_ERROR_CHECK(rmt_driver_install(dmaChannelRX, 0, 0));
	rmt_rx_start(dmaChannelRX, true);

    lastCounterReset = millis();
}


ESC::~ESC(){
    rmt_driver_uninstall(this->dmaChannelTX);
}

#define INVALID_ERPM 0xfffff
uint32_t bufToERPM(volatile rmt_item32_t items[64], int startPos, int length){
    if (length <= startPos) return INVALID_ERPM;
	static int pos = 20;
	static uint32_t value = 0x1fffff;

	//read 21 bit double encoded value from rmt buffer
	for (int i = startPos; i < length; i++){
		int dur0 = items[i].duration0 + 15;
		int dur1 = items[i].duration1 + 15;
		dur0 /= 105;
		dur1 /= 105;
		for (int i = 0; i < dur0; i++){
			if (pos < 0){
				return INVALID_ERPM;
			}
			value ^= 1 << pos--;
		}
		pos -= dur1;
	}

	//perform 21 bit -> 20 bit decoding
	value = (value >> 1) ^ value;

	#define iv 0xffffffff
    value &= 0xfffff;
    static const uint32_t decode[32] = {
        iv, iv, iv, iv, iv, iv, iv, iv, iv, 9, 10, 11, iv, 13, 14, 15,
        iv, iv, 2, 3, iv, 5, 6, 7, iv, 0, 8, 1, iv, 4, 12, iv };

	//perform GCR decoding
    static uint32_t decodedValue = decode[value & 0x1f];
    decodedValue |= decode[(value >> 5) & 0x1f] << 4;
    decodedValue |= decode[(value >> 10) & 0x1f] << 8;
    decodedValue |= decode[(value >> 15) & 0x1f] << 12;
	
	//calculate checksum
    static uint32_t csum = decodedValue;
    csum = csum ^ (csum >> 8);
    csum = csum ^ (csum >> 4);

    if ((csum & 0xf) != 0xf || decodedValue > 0xffff) {
		// intrCounter++;
		return INVALID_ERPM;

    } else {
        value = decodedValue >> 4;
        if (value == 0x0fff) {
            return 0;
        }
        value = (value & 0x000001ff) << ((value >> 9) & 0x7);
        if (!value) {
            return INVALID_ERPM;
        }
        value = (1000000 * 60 + value * 50) / value;
    }
	// return value / (MOTOR_POLES / 2)/60;
    return value;
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

bool ESC::loop(){
    bool newTelemetry = false;

	RingbufHandle_t handle;
	ESP_ERROR_CHECK(rmt_get_ringbuf_handle(dmaChannelRX, &handle));
	volatile rmt_item32_t* itemPtr = RMTMEM.chan[dmaChannelRX].data32;
	if (lastSend && micros() - lastSend > 150){
		lastSend = 0;
		memcpy(items, (const void *)itemPtr, 4*64);
		eRPM = bufToERPM(itemPtr, 16, rmt_get_mem_len(dmaChannelRX));
		arb = rmt_get_mem_len(dmaChannelRX);

		uint32_t intr_st = RMT.int_st.val;
		auto& conf = RMT.conf_ch[dmaChannelRX].conf1;
		conf.rx_en = 0;
		conf.mem_owner = RMT_MEM_OWNER_TX;

		conf.mem_wr_rst = 1;
		conf.mem_owner = RMT_MEM_OWNER_RX;
		RMT.int_clr.val = intr_st;
		conf.rx_en = 1;
		if (eRPM == INVALID_ERPM || (arb < 20 && arb > 16)){
			eRPM = 0;
			return false;
		}
		erpmCounter++;
		newTelemetry = true;
	}

    loopCounter++;
    if (millis() - lastCounterReset >= 1000){
        lastCounterReset = millis();
        sendFreq = sendCounter;
        loopFreq = loopCounter;
		erpmFreq = erpmCounter;
        sendCounter = 0;
        loopCounter = 0;
		erpmCounter = 0;
    }
    return newTelemetry;
}


void ESC::arm(bool arm){
    armed=arm;
    nextThrottle = 0;
    currentThrottle = 0;
}


void ESC::beep(uint8_t level, bool pauseBefore){
    uint8_t start = manualDataAmount;
    if (pauseBefore){
        while (start < 50){
            this->manualData11[start] = 0;
            start++;
        }
    }
    switch(level){
        case ESC_BEEP_1:
            manualData11[start++] = CMD_BEACON1;
            break;
        case ESC_BEEP_2:
            manualData11[start++] = CMD_BEACON2;
            break;
        case ESC_BEEP_3:
            manualData11[start++] = CMD_BEACON3;
            break;
        case ESC_BEEP_4:
            manualData11[start++] = CMD_BEACON4;
            break;
        case ESC_BEEP_5:
            manualData11[start++] = CMD_BEACON5;
            break;
    }
    manualDataAmount = start;
}


void ESC::setThrottle(uint16_t newThrottle){
    newThrottle = (newThrottle < 0) ? 0 : newThrottle;
    nextThrottle = (newThrottle > maxThrottle) ? maxThrottle : newThrottle;
}

void ESC::send(){
    currentThrottle = nextThrottle;
    if (manualDataAmount){
        sendRaw11(manualData11[0]);
        manualDataAmount--;
        for (uint8_t i = 0; i < manualDataAmount; i++){
            manualData11[i] = manualData11[i+1];
        }
    } else {
        if (armed){
            sendThrottle(currentThrottle);
        } else {
            sendThrottle(0);
        }
    }
}



void ESC::sendThrottle(uint16_t throttleValue){
    if (throttleValue) throttleValue += 47;
    sendFullRaw(appendChecksum(throttleValue << 1 | true));
}


void ESC::sendRaw11(uint16_t rawValueWithoutChecksum){
    sendFullRaw(appendChecksum(rawValueWithoutChecksum << 1 | true));
}


void ESC::sendFullRaw(uint16_t rawValueWithChecksum){
    // rmt_set_pin(dmaChannelTX, RMT_MODE_TX, (gpio_num_t)signalPin);
    // delayMicroseconds(10);
    uint16_t mask = 1 << 15;
    for (uint8_t bit = 0; bit < ESC_BUFFER_ITEMS; bit++) {
        uint16_t bit_is_set = rawValueWithChecksum & mask;
        dataBuffer[bit] = bit_is_set ? (rmt_item32_t) {{{T1L, 0, T1H, 1}}} : (rmt_item32_t) {{{T0L, 0, T0H, 1}}};
        mask >>= 1;
    }
    ESP_ERROR_CHECK(rmt_write_items(this->dmaChannelTX, dataBuffer, ESC_BUFFER_ITEMS, false));
    sendCounter++;
	lastSend = micros();
}

void ESC::setMaxThrottle (uint16_t maxThrottle){
    if (maxThrottle > 2000)
        ESC::maxThrottle = 2000;
    else
        ESC::maxThrottle = maxThrottle;
    
}


uint16_t ESC::getMaxThrottle (){
    return ESC::maxThrottle;
}


uint16_t ESC::appendChecksum(uint16_t value){
    int csum = 0, csum_data = value;
    for (int i = 0; i < 3; i++) {
        csum ^=  csum_data;
        csum_data >>= 4;
    }
    csum = ~csum;
    csum &= 0xf;
    value = (value << 4) | csum;
    return value;
}



uint8_t update_crc8(uint8_t crc, uint8_t crc_seed){
  uint8_t crc_u, i;
  crc_u = crc;
  crc_u ^= crc_seed;
  for ( i=0; i<8; i++)
    crc_u = ( crc_u & 0x80 ) ? 0x7 ^ ( crc_u << 1 ) : ( crc_u << 1 );
  return (crc_u);
}

uint8_t get_crc8(uint8_t *Buf, uint8_t BufLen){
  uint8_t crc = 0, i;
  for( i=0; i<BufLen; i++)
    crc = update_crc8(Buf[i], crc);
  return (crc);
}