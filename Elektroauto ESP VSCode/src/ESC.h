//! @file ESC.h Driver for DShot ESCs

#include <Arduino.h>
#include <driver/rmt.h>

#ifndef ESC_H
#define ESC_H

///DShot 150: 12, DShot 300: 6, DShot 600: 3, change value accordingly for the desired DShot speed
#define CLK_DIV 3
/// 0 bit high time
#define T0H 17
/// 1 bit high time
#define T1H 33
/// 0 bit low time
#define T0L 27
/// 1 bit low time
#define T1L 11
/// reset length in multiples of bit time
#define T_RESET 21
/// number of bits sent out via DShot
#define ESC_BUFFER_ITEMS 16
/// maximum amount of manual data
#define MAX_MANUAL_DATA 256

#define ARMED_MASK      (uint8_t)0b00100000
#define ENABLED_MASK    (uint8_t)0b00010000
#define CONNECTED_MASK  (uint8_t)0b00001000
#define RED_LED_MASK    (uint8_t)0b00000100
#define GREEN_LED_MASK  (uint8_t)0b00000010
#define BLUE_LED_MASK   (uint8_t)0b00000001

#define DEFAULT_STATUS  (ENABLED_MASK | RED_LED_MASK | GREEN_LED_MASK | BLUE_LED_MASK)

enum {
    CMD_MOTOR_STOP = 0,
    CMD_BEACON1,
    CMD_BEACON2,
    CMD_BEACON3,
    CMD_BEACON4,
    CMD_BEACON5,
    CMD_ESC_INFO, // V2 includes settings
    CMD_SPIN_DIRECTION_1,
    CMD_SPIN_DIRECTION_2,
    CMD_3D_MODE_OFF,
    CMD_3D_MODE_ON,
    CMD_SETTINGS_REQUEST, // Currently not implemented
    CMD_SAVE_SETTINGS,
    CMD_SPIN_DIRECTION_NORMAL = 20,
    CMD_SPIN_DIRECTION_REVERSED = 21,
    CMD_LED0_ON, // BLHeli32 only
    CMD_LED1_ON, // BLHeli32 only
    CMD_LED2_ON, // BLHeli32 only
    CMD_LED3_ON, // BLHeli32 only
    CMD_LED0_OFF, // BLHeli32 only
    CMD_LED1_OFF, // BLHeli32 only
    CMD_LED2_OFF, // BLHeli32 only
    CMD_LED3_OFF, // BLHeli32 only
    CMD_MAX = 47
};

enum{
    ERROR_OVERHEAT = 0,
    ERROR_TOO_FAST,
    ERROR_VOLTAGE_LOW
};
enum{
    ESC_BEEP_1 = 1,
    ESC_BEEP_2,
    ESC_BEEP_3,
    ESC_BEEP_4,
    ESC_BEEP_5
};

class ESC {
private:
    bool isTelemetryComplete();

    HardwareSerial *telemetryStream;
    uint8_t telemetryPin;
    rmt_channel_t dmaChannel;
    void (*onESCError) (ESC *esc, uint8_t errorCode);
    void (*onStatusChange) (ESC *esc, uint8_t newStatus, uint8_t oldStatus);

    uint16_t noTelemetryCounter = 0;
    uint8_t pStatus = DEFAULT_STATUS;
    char telemetry[10];
    rmt_item32_t dataBuffer[ESC_BUFFER_ITEMS];
    double nextThrottle = 0;

    static uint16_t maxThrottle;
public:
    ESC(HardwareSerial *telemetryStream, int8_t signalPin, int8_t telemetryPin, rmt_channel_t dmaChannel, void (*onError) (ESC *esc, uint8_t errorCode), void (*onStatusChange) (ESC *esc, uint8_t newStatus, uint8_t oldStatus));
    ~ESC();
    bool loop();
    void pause();
    void resume();
    void arm(bool arm);
    void beep(uint8_t level, bool pauseBefore = false);
    void setThrottle(double newThrottle);
    void IRAM_ATTR send();
    void IRAM_ATTR sendThrottle(uint16_t throttleValue);
    void IRAM_ATTR sendRaw11(uint16_t rawValueWithoutChecksum);
    void IRAM_ATTR sendFullRaw(uint16_t rawValueWithChecksum);
    bool getRedLED();
    bool getGreenLED();
    bool getBlueLED();
    static void setMaxThrottle (uint16_t maxThrottle);
    static uint16_t getMaxThrottle ();
    static uint16_t appendChecksum(uint16_t value);
    unsigned long lastTelemetry = 0;
    uint16_t voltage = 0;
    uint16_t heRPM = 0;
    uint16_t speed = 0;
    uint8_t temperature = 0;
    uint8_t status = DEFAULT_STATUS;
    uint16_t manualData11[MAX_MANUAL_DATA];
    uint16_t manualDataAmount = 0;
    double currentThrottle = 0;
    static uint16_t cutoffVoltage;
};

#endif