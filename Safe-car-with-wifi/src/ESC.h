//! @file ESC.h Driver for DShot ESCs

#include <Arduino.h>
#include <driver/rmt.h>

#ifndef ESC_H
#define ESC_H

///DShot 150: 8, DShot 300: 4, DShot 600: 2, change value accordingly for the desired DShot speed
#define CLK_DIV 2
/// 0 bit high time
#define T0H 42
/// 1 bit high time
#define T1H 17
/// 0 bit low time
#define T0L 25
/// 1 bit low time
#define T1L 50
/// number of bits sent out via DShot
#define ESC_BUFFER_ITEMS 16
/// maximum amount of manual data
#define MAX_MANUAL_DATA 256

/// special commands
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

/// beep codes
enum{
    ESC_BEEP_1 = 1,
    ESC_BEEP_2,
    ESC_BEEP_3,
    ESC_BEEP_4,
    ESC_BEEP_5
};

/**
 * @brief represents one ESC/motor combo
 * 
 * use methods to read and write data, call loop() regularly to receive telemetry
 */
class ESC {
private:
    /// buffer for sending ESC packets
    rmt_item32_t dataBuffer[ESC_BUFFER_ITEMS];
    /// next throttle
    uint16_t nextThrottle = 0;

    /// the maximum throttle for all ESCs
    static uint16_t maxThrottle;

    uint32_t sendCounter = 0;
    uint32_t loopCounter = 0;
    uint32_t lastCounterReset = 0;
public:
    /**
     * @brief registers a new ESC
     * 
     * @param signalPin pin for data output (DShot)
     * @param dmaChannelTX dmaChannel for sending the packets
     */
    ESC(int8_t signalPin, rmt_channel_t dmaChannelTX);
    /// @brief destroys the ESC object
    ~ESC();
    /** @brief checks for telemetry
     * 
     * reads errors (onError), supplies telemetry to telemetry variables, checks if ESC is connected, checks for status change (onStatusChange)
     * @return true if new telemetry
     */
    bool loop();
    /// @brief arms the ESC (throttle is set to 0)
    void arm(bool arm);
    /**
     * @brief beeps the ESC once
     * 
     * @param level the beeping level (from enum)
     * @param pauseBefore whether to pause for some ms before beeping
     */
    void beep(uint8_t level, bool pauseBefore = false);
    /**
     * @brief set new Throttle (0...2000)
     * 
     * use this method for normal control. Use send() every cycle. This method also checks for too high throttle (ESC::maxThrottle)
     * @param newThrottle the new throttle value
     */
    void setThrottle(uint16_t newThrottle);
    /**
     * @brief This method will write the newThrottle to currentThrottle and send the newThrottle to the ESC
     * 
     * If manual data is present, it will send manualData instead, until it is empty<br>implements safety measures like cutoffVoltage
     */
    void send();
    /**
     * @brief sends a throttle value immediately, no checks, no telemetry request
     * 
     * DO NOT USE THIS FOR NORMAL OPERATION. USE send() and setThrottle() INSTEAD.
     * @param throttleValue the throttle value (0...2000)
     */
    void sendThrottle(uint16_t throttleValue);
    /**
     * @brief sends an 11 bit raw value immediately, no checks, but telemetry request
     * 
     * DO NOT USE THIS FOR NORMAL OPERATION. USE send() and setThrottle()/manualData INSTEAD.
     * @param rawValueWithoutChecksum raw 11 bit value (right aligned)
     */
    void sendRaw11(uint16_t rawValueWithoutChecksum);
    /**
     * @brief sends a 16 bit raw value immediately, no checks, telemetry bit is set by user
     * 
     * DO NOT USE THIS FOR NORMAL OPERATION. USE send() and setThrottle()/manualData INSTEAD, UNLESS TELEMETRY BIT DOESN'T ALLOW.
     * @param rawValueWithChecksum raw 16 bit value
     */
    void sendFullRaw(uint16_t rawValueWithChecksum);
    /**
     * @brief Set the maximum throttle of all ESCs
     * 
     * limits maxThrottle to 2000
     * @param maxThrottle new maximum Throttle
     */
    static void setMaxThrottle (uint16_t maxThrottle);
    /**
     * @brief Get the maximum throttle of all ESCs
     * 
     * @return uint16_t maximum throttle
     */
    static uint16_t getMaxThrottle ();
    /**
     * @brief shifts a uint16_t 4 bits left and appends the CRC checksum
     * 
     * @param value the (right aligned) value (12 bit)
     * @return uint16_t the final bit array that can be sent to the ESC
     */
    static uint16_t appendChecksum(uint16_t value);
    /// armed
    uint8_t armed = false;
    /// manual data array, put data here and set manualDataAmount
    uint16_t manualData11[MAX_MANUAL_DATA];
    /// last useful index of manualData11 +1, so for 3 manual data packets, write 3 here
    uint16_t manualDataAmount = 0;
    ///currentThrottle (read only)
    uint16_t currentThrottle = 0;
    /// dma channel used for transmission
    rmt_channel_t dmaChannelTX = (rmt_channel_t)0;
    /// dma channel used for bidirectional dshot
    rmt_channel_t dmaChannelRX = (rmt_channel_t)0;
    ///
    uint32_t sendFreq = 0;
    uint32_t loopFreq = 0;
    uint8_t signalPin = 0;
};

#endif