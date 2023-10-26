#include "elapsedMillis.h"
#include <Arduino.h>
#include <deque>
using std::deque;
#define START_BUTTON_PIN 5

enum crsf_commands {
    CRSF_FRAMETYPE_LINK_STATISTICS          = 0x14,
    CRSF_FRAMETYPE_RC_CHANNELS_PACKED       = 0x16,
    CRSF_FRAMETYPE_DEVICE_PING              = 0x28,
    CRSF_FRAMETYPE_DEVICE_INFO              = 0x29,
    CRSF_FRAMETYPE_PARAMETER_SETTINGS_ENTRY = 0x2B,
    CRSF_FRAMETYPE_PARAMETER_READ           = 0x2C,
    CRSF_FRAMETYPE_PARAMETER_WRITE          = 0x2D,
    CRSF_FRAMETYPE_COMMAND                  = 0x32
};
enum crsf_addresses {
    CRSF_ADDRESS_CRSF_TRANSMITTER  = 0xEE,
    CRSF_ADDRESS_RADIO_TRANSMITTER = 0xEA,
    CRSF_ADDRESS_FLIGHT_CONTROLLER = 0xC8,
    CRSF_ADDRESS_CRSF_RECEIVER     = 0xEC
};
typedef struct {
    int16_t  millis; // first frame with millis = -1 defines the end of the sequence
    uint16_t value;  // 1000 - 2000
} interpolation_frame;

interpolation_frame frames[4][10] = {
    {},
    {{0, 1500}, {300, 1500}, {400, 1200}, {1300, 2000}, {-1, 0}},
    {{0, 1500}, {300, 1500}, {400, 1200}, {1300, 2000}, {-1, 0}},
    {}};

bool pinged = false;

elapsedMillis sinceLastPing;

deque<uint8_t> rxBuffer;
elapsedMillis  sinceLastRC;
uint16_t       outChannels[4] = {0};
elapsedMillis  sinceLaunch;
bool           launched = false;

void crc32_append(uint32_t data, uint32_t &crc) {
    crc ^= data;
    for (uint32_t i = 0; i < 8; i++) {
        if (crc & 0x80) {
            crc = (crc << 1) ^ 0xD5;
        } else {
            crc <<= 1;
        }
    }
}

void sendCRSFCommand(uint8_t dest, uint8_t len, uint8_t type, uint8_t *data) {
    Serial2.write(dest);
    Serial2.write(len + 2);
    Serial2.write(type);
    uint32_t crc = 0;
    crc32_append(type, crc);
    for (int i = 0; i < len; i++) {
        Serial2.write(data[i]);
        crc32_append(data[i], crc);
    }
    Serial2.write(crc & 0xFF);
}

void setup() {
    Serial.begin(921600);
    Serial.println("Initializing");
    Serial2.begin(921600, SERIAL_8N1, -1, -1, true); // 16: RX, 17: TX
    pinMode(START_BUTTON_PIN, INPUT_PULLUP);
}

void handleCRSFStream() {
    uint8_t dest     = rxBuffer[0];
    uint8_t len      = rxBuffer[1];
    uint8_t type     = rxBuffer[2];
    uint8_t checksum = 0;
    if ((rxBuffer[0] != CRSF_ADDRESS_RADIO_TRANSMITTER && rxBuffer[0] != 0x00) || len > 62 || len < 2) { // 0x00 is broadcast
        // Serial.println("Wrong destination");
        // Serial.println(rxBuffer[0], HEX);
        // if (rxBuffer[0] == CRSF_ADDRESS_RADIO_TRANSMITTER) {
        //     Serial.printf("len: %02X, type: %02X\n", len, type);
        // }
        rxBuffer.pop_front();
        return;
    }
    if (rxBuffer.size() < len + 2) return;
    uint32_t crc = 0;
    for (int i = 0; i < len; i++)
        crc32_append(rxBuffer[i + 2], crc);

    if (checksum & 0xFF) {
        Serial.println("Checksum error");
        for (int i = 0; i < len + 2; i++) {
            Serial.printf("%02X ", rxBuffer[i]);
        }
        Serial.println();
        rxBuffer.pop_front();
        return;
    }
    const uint8_t *data = &rxBuffer[3];
    switch (type) {
    case CRSF_FRAMETYPE_LINK_STATISTICS:
        Serial.println("Link statistics, implementation todo");
        break;
    case CRSF_FRAMETYPE_RC_CHANNELS_PACKED:
        Serial.println("RC channels, wtf");
        if (len == 24) {
            uint64_t decoder, decoder2;
            memcpy(&decoder, &rxBuffer[3], 8);
            uint32_t pChannels[16];
            pChannels[0] = decoder & 0x7FF;         // 0...10
            pChannels[1] = (decoder >> 11) & 0x7FF; // 11...21
            pChannels[2] = (decoder >> 22) & 0x7FF; // 22...32
            pChannels[3] = (decoder >> 33) & 0x7FF; // 33...43
            pChannels[4] = (decoder >> 44) & 0x7FF; // 44...54
            decoder >>= 55;                         // 55, 9 bits left
            memcpy(&decoder2, &rxBuffer[11], 6);
            decoder |= (decoder2 << 9);             // 57 bits left
            pChannels[5] = decoder & 0x7FF;         // 55...65
            pChannels[6] = (decoder >> 11) & 0x7FF; // 66...76
            pChannels[7] = (decoder >> 22) & 0x7FF; // 77...87
            pChannels[8] = (decoder >> 33) & 0x7FF; // 88...98
            pChannels[9] = (decoder >> 44) & 0x7FF; // 99...109
            decoder >>= 55;                         // 55, 2 bits left
            memcpy(&decoder2, &rxBuffer[17], 7);
            decoder |= (decoder2 << 2);              // 58 bits left
            pChannels[10] = decoder & 0x7FF;         // 110...120
            pChannels[11] = (decoder >> 11) & 0x7FF; // 121...131
            pChannels[12] = (decoder >> 22) & 0x7FF; // 132...142
            pChannels[13] = (decoder >> 33) & 0x7FF; // 143...153
            pChannels[14] = (decoder >> 44) & 0x7FF; // 154...164
            decoder >>= 55;                          // 55, 3 bits left
            pChannels[15] = decoder | (rxBuffer[24] << 3);
            // for (int i = 0; i < 4; i++) {
            //     Serial.printf("%5d, ", pChannels[i]);
            // }
            // Serial.println();
        }
        break;
    case CRSF_FRAMETYPE_DEVICE_PING:
        Serial.println("Ping, implementation todo");
        break;
    case CRSF_FRAMETYPE_DEVICE_INFO:
        Serial.println("Got device info: ");
        if (data[0] == 0x00 || data[0] == CRSF_ADDRESS_RADIO_TRANSMITTER) {
            uint8_t deviceType = data[1];
            switch (deviceType) {
            case CRSF_ADDRESS_RADIO_TRANSMITTER:
                Serial.println("  Type: handset, wtf");
                break;
            case CRSF_ADDRESS_FLIGHT_CONTROLLER:
                Serial.println("  Type: FC");
                break;
            case CRSF_ADDRESS_CRSF_RECEIVER:
                Serial.println("  Type: RX Module");
                break;
            case CRSF_ADDRESS_CRSF_TRANSMITTER:
                Serial.println("  Type: TX Module");
                break;
            }
            Serial.printf("  Name: %s\n", &data[2]);
            const uint8_t *moreData = &data[2 + strlen((char *)&data[2]) + 1];
            Serial.printf("  Serial Number: %c%c%c%c\n", moreData[0], moreData[1], moreData[2], moreData[3]);
            Serial.printf("  Hardware Version: %02X, %02X, %02X, %02X\n", moreData[4], moreData[5], moreData[6], moreData[7]);
            Serial.printf("  Software Version: %02X, %02X, %02X, %02X\n", moreData[8], moreData[9], moreData[10], moreData[11]);
            Serial.printf("  Config Parameters: %d\n", moreData[12]);
            Serial.printf("  Protocol Version: %d\n", moreData[13]);
            pinged = true;
        } else {
            Serial.println("but not for this device");
        }
        break;
    case CRSF_FRAMETYPE_PARAMETER_SETTINGS_ENTRY:
        Serial.println("Parameter settings entry");
        break;
    case CRSF_FRAMETYPE_PARAMETER_READ:
        Serial.println("Parameter read");
        break;
    case CRSF_FRAMETYPE_PARAMETER_WRITE:
        Serial.println("Parameter write");
        break;
    case CRSF_FRAMETYPE_COMMAND:
        Serial.println("Command");
        break;
    default:
        Serial.println("Unknown");
        for (int i = 0; i < len + 2; i++) {
            Serial.printf("%02X ", rxBuffer[i]);
        }
        Serial.println();
        break;
    }
    rxBuffer.erase(rxBuffer.begin(), rxBuffer.begin() + len + 2);
}

void updateChannels() {
    for (int i = 0; i < 4; i++) {
        if (frames[i][0].millis == -1) {
            outChannels[i] = 172;
            continue;
        }
        int j = 0;
        for (; j < 10; j++) {
            if (frames[i][j].millis == -1) {
                outChannels[i] = frames[i][j - 1].value;
                break;
            }
            if (frames[i][j].millis > sinceLaunch) {
                if (j == 0) {
                    outChannels[i] = frames[i][j].value;
                } else {
                    float t        = (float)(sinceLaunch - frames[i][j - 1].millis) / (float)(frames[i][j].millis - frames[i][j - 1].millis);
                    outChannels[i] = frames[i][j - 1].value + (frames[i][j].value - frames[i][j - 1].value) * t;
                    outChannels[i] = map(outChannels[i], 1000, 2000, 172, 1882);
					Serial.println(outChannels[i]);
                }
                break;
            }
        }
        if (j == 10) {
            outChannels[i] = frames[i][9].value;
        }
    }
}

void loop() {
    if (!pinged && sinceLastPing > 100) {
        uint8_t data[2] = {0x00, CRSF_ADDRESS_RADIO_TRANSMITTER};
        sendCRSFCommand(CRSF_ADDRESS_CRSF_TRANSMITTER, 2, CRSF_FRAMETYPE_DEVICE_PING, data);
        sinceLastPing = 0;
    }
    while (Serial2.available()) {
        rxBuffer.push_back(Serial2.read());
    }
    if (rxBuffer.size() > 2) {
        handleCRSFStream();
    }
    if (digitalRead(START_BUTTON_PIN) == LOW) {
        if (!launched) {
            launched    = true;
            sinceLaunch = 0;
        }
    }
    if (sinceLaunch > 2000) {
        launched = false;
    }
    if (sinceLastRC > 3) {
        if (launched) {
            updateChannels();
        } else {
            outChannels[0] = 172;
            outChannels[1] = 172;
            outChannels[2] = 172;
            outChannels[3] = 172;
        }

        uint64_t data[3] = {0};

        data[0] = outChannels[0] & 0x7FF | (outChannels[1] & 0x7FF) << 11 | (uint64_t)(outChannels[2] & 0x7FF) << 22 | (uint64_t)(outChannels[3] & 0x7FF) << 33;
        sendCRSFCommand(CRSF_ADDRESS_CRSF_TRANSMITTER, 22, CRSF_FRAMETYPE_RC_CHANNELS_PACKED, (uint8_t *)data);
        sinceLastRC = 0;
    }
}