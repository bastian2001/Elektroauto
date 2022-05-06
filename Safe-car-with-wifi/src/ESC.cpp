#include "Arduino.h"
#include "driver/rmt.h"
#include "ESC.h"

uint8_t get_crc8(uint8_t *Buf, uint8_t BufLen);

uint16_t ESC::maxThrottle = 2000;

rmt_isr_handle_t xHandler = NULL;
volatile rmt_item32_t itemBuf[64];


ESC::ESC(int8_t signalPin, rmt_channel_t dmaChannelTX)
: dmaChannelTX(dmaChannelTX)
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
    c.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
    c.tx_config.idle_output_en = true;
    ESP_ERROR_CHECK(rmt_config(&c));
    ESP_ERROR_CHECK(rmt_driver_install(dmaChannelTX, 0, 0));

    for (int i = 0; i < MAX_MANUAL_DATA; i++){
        manualData11[i] = 0;
    }

    lastCounterReset = millis();
}


ESC::~ESC(){
    rmt_driver_uninstall(this->dmaChannelTX);
    rmt_driver_uninstall(this->dmaChannelRX);
}

bool ESC::loop(){
    bool newTelemetry = false;

    loopCounter++;
    if (millis() - lastCounterReset >= 1000){
        lastCounterReset = millis();
        sendFreq = sendCounter;
        loopFreq = loopCounter;
        sendCounter = 0;
        loopCounter = 0;
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
    Serial.println(rawValueWithoutChecksum, HEX);
    sendFullRaw(appendChecksum(rawValueWithoutChecksum << 1 | true));
}


void ESC::sendFullRaw(uint16_t rawValueWithChecksum){
    uint16_t mask = 1 << 15;
    for (uint8_t bit = 0; bit < ESC_BUFFER_ITEMS; bit++) {
        uint16_t bit_is_set = rawValueWithChecksum & mask;
        dataBuffer[bit] = bit_is_set ? (rmt_item32_t) {{{T1H, 1, T1L, 0}}} : (rmt_item32_t) {{{T0H, 1, T0L, 0}}};
        mask >>= 1;
    }
    ESP_ERROR_CHECK(rmt_write_items(this->dmaChannelTX, dataBuffer, ESC_BUFFER_ITEMS, false));
    sendCounter++;
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
    // csum = ~csum;
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