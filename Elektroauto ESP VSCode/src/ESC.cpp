#include "Arduino.h"
#include "driver/rmt.h"
#include "ESC.h"

uint8_t get_crc8(uint8_t *Buf, uint8_t BufLen);

uint16_t ESC::maxThrottle = 2000;
uint16_t ESC::cutoffVoltage = 640;

ESC::ESC(HardwareSerial *telemetryStream, int8_t signalPin, int8_t telemetryPin, rmt_channel_t dmaChannel, void (*onError) (ESC *esc, uint8_t errorCode), void (*onStatusChange) (ESC *esc, uint8_t newStatus, uint8_t oldStatus))
: telemetryStream(telemetryStream)
, telemetryPin(telemetryPin)
, dmaChannel(dmaChannel)
, onESCError(onError)
, onStatusChange(onStatusChange){
    telemetryStream->begin(115200, SERIAL_8N1, telemetryPin);
    status |= ENABLED_MASK;

    rmt_config_t c;
    c.rmt_mode = RMT_MODE_TX;
    c.channel = dmaChannel;
    c.clk_div = CLK_DIV;
    c.gpio_num = (gpio_num_t) signalPin;
    c.mem_block_num = 1;
    c.tx_config.loop_en = false;
    c.tx_config.carrier_en = false;
    c.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
    c.tx_config.idle_output_en = true;
    ESP_ERROR_CHECK(rmt_config(&c));
    ESP_ERROR_CHECK(rmt_driver_install(dmaChannel, 0, 0));
}


ESC::~ESC(){
    rmt_driver_uninstall(this->dmaChannel);
}


bool ESC::loop(){
    bool newTelemetry = false;

    if (noTelemetryCounter > 50 && (status & CONNECTED_MASK) && (status & ENABLED_MASK)){
        status &= ((uint8_t) 0xFF - CONNECTED_MASK);
    }
    // Serial.print('1');
    while (this->telemetryStream->available()){
        for (uint8_t i = 0; i < 9; i++){
            telemetry[i] = telemetry[i+1];
        }
        telemetry[9] = (char) telemetryStream->read();
        if (isTelemetryComplete()){
            noTelemetryCounter = 0;

            if (!(status & CONNECTED_MASK)){
                for (uint8_t i = 0; i < 17; i++){
                    manualData11[i] = 0;
                }
                manualData11[17] = (status & RED_LED_MASK) ? CMD_LED0_ON : CMD_LED0_OFF;
                manualData11[18] = (status & GREEN_LED_MASK) ? CMD_LED1_ON : CMD_LED1_OFF;
                manualData11[19] = (status & BLUE_LED_MASK) ? CMD_LED2_ON : CMD_LED2_OFF;
                manualDataAmount = 20;
                status |= CONNECTED_MASK;
            }

            temperature = telemetry[0];
            voltage = (telemetry[1] << 8) | telemetry[2];
            heRPM = (telemetry[7] << 8) | telemetry[8];
            speed = (float)heRPM;// * erpmToMMPerSecond;
            for (uint8_t i = 0; i < 10; i++){
                telemetry[i] = 1;
            }

            if (temperature > 80){
                this->onESCError(this, ERROR_OVERHEAT);
            } else if(voltage < cutoffVoltage){
                this->onESCError(this, ERROR_VOLTAGE_LOW);
            } else if (heRPM > 8000){
                this->onESCError(this, ERROR_TOO_FAST);
            }
            newTelemetry = true;
            break;
        }
    }

    if (status != pStatus){
        this->onStatusChange(this, status, pStatus);
        pStatus = status;
    }

    return newTelemetry;
}


bool ESC::isTelemetryComplete(){
    if (telemetry[0] > 1
        && get_crc8((uint8_t*)telemetry, 9) == telemetry[9]
        && telemetry[3] == 0
        && telemetry[4] == 0
        && telemetry[5] == 0
        && telemetry[6] == 0
        && telemetry[0] < 100
        && telemetry[1] < 8
    )
        return true;
    return false;
}


void ESC::pause(){
    telemetryStream->end();
    status &= (0xFF - ENABLED_MASK);
}


void ESC::resume(){
    telemetryStream->begin(115200, SERIAL_8N1, telemetryPin);
    noTelemetryCounter = 0;
    status |= ENABLED_MASK;
}


void ESC::arm(bool arm){
    if (arm){
        status |= ARMED_MASK;
    } else {
        status &= (0xFF - ARMED_MASK);
    }
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


void ESC::setThrottle(double newThrottle){
    newThrottle = (newThrottle < 0) ? 0 : newThrottle;
    nextThrottle = (newThrottle > maxThrottle) ? maxThrottle : newThrottle;
}

void IRAM_ATTR ESC::send(){
    currentThrottle = nextThrottle;
    if (manualDataAmount){
        sendRaw11(manualData11[0]);
        manualDataAmount--;
        for (uint8_t i = 0; i < manualDataAmount; i++){
            manualData11[i] = manualData11[i+1];
        }
    } else {
        if (status & ARMED_MASK){
            sendThrottle(currentThrottle + .5);
        } else {
            sendThrottle(0);
        }
    }
}


void IRAM_ATTR ESC::sendThrottle(uint16_t throttleValue){
    if (throttleValue) throttleValue += 47;
    sendRaw11(throttleValue);
}


void IRAM_ATTR ESC::sendRaw11(uint16_t rawValueWithoutChecksum){
    sendFullRaw(appendChecksum(rawValueWithoutChecksum << 1 | true));
}


void IRAM_ATTR ESC::sendFullRaw(uint16_t rawValueWithChecksum){
    uint16_t mask = 1 << (ESC_BUFFER_ITEMS - 1);
    for (uint8_t bit = 0; bit < ESC_BUFFER_ITEMS; bit++) {
        uint16_t bit_is_set = rawValueWithChecksum & mask;
        dataBuffer[bit] = bit_is_set ? (rmt_item32_t) {{{T1H, 1, T1L, 0}}} : (rmt_item32_t) {{{T0H, 1, T0L, 0}}};
        mask >>= 1;
    }
    ESP_ERROR_CHECK(rmt_write_items(this->dmaChannel, dataBuffer, ESC_BUFFER_ITEMS, false));
    switch(rawValueWithChecksum){
        case 0x0356:
            status &= (unsigned char)0xFF - RED_LED_MASK;
            break;
        case 0x02DF:
            status |= RED_LED_MASK;
            break;
        case 0x0374:
            status &= (unsigned char)0xFF - GREEN_LED_MASK;
            break;
        case 0x02FD:
            status |= GREEN_LED_MASK;
            break;
        case 0x039A:
            status &= (unsigned char)0xFF - BLUE_LED_MASK;
            break;
        case 0x0312:
            status |= BLUE_LED_MASK;
            break;
        default:
            break;
  }
}


bool ESC::getRedLED(){
    return (this->status & RED_LED_MASK) > 0;
}


bool ESC::getGreenLED(){
    return (this->status & GREEN_LED_MASK) > 0;
}


bool ESC::getBlueLED(){
    return (this->status & BLUE_LED_MASK) > 0;
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


uint16_t IRAM_ATTR ESC::appendChecksum(uint16_t value){
    int csum = 0, csum_data = value;
    for (int i = 0; i < 3; i++) {
        csum ^=  csum_data;
        csum_data >>= 4;
    }
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