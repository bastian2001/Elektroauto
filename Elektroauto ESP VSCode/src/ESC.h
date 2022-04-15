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

/// status bit armed
#define ARMED_MASK      (uint8_t)0b00100000
/// status bit ESC enabled
#define ENABLED_MASK    (uint8_t)0b00010000
/// status bit ESC connected (based on telemetry)
#define CONNECTED_MASK  (uint8_t)0b00001000
/// red LED on?
#define RED_LED_MASK    (uint8_t)0b00000100
/// green LED on?
#define GREEN_LED_MASK  (uint8_t)0b00000010
/// blue LED on?
#define BLUE_LED_MASK   (uint8_t)0b00000001

#define DEFAULT_STATUS  (ENABLED_MASK | RED_LED_MASK | GREEN_LED_MASK | BLUE_LED_MASK)

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

/// error codes
enum{
    ERROR_OVERHEAT = 0,
    ERROR_TOO_FAST,
    ERROR_VOLTAGE_LOW
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
    /// checks if telemetry is complete
    bool isTelemetryComplete();

    /// stores the stream for telemetry readout, NULL if none
    // HardwareSerial *telemetryStream;
    /// pin for telemetry readout
    // uint8_t telemetryPin;
    /// ESC error method, this is fired when an error occurs
    void (*onESCError) (ESC *esc, uint8_t errorCode);
    /// ESC status change, this is fired when the status changes (e.g. ESC is connected)
    void (*onStatusChange) (ESC *esc, uint8_t newStatus, uint8_t oldStatus);

    /// counter for no received telemetry
    unsigned long disconnectedAt = 0;
    /// the previous status (used for status change detection)
    uint8_t pStatus = DEFAULT_STATUS;
    /// telemetry storage
    char telemetry[10];
    /// buffer for sending ESC packets
    rmt_item32_t dataBuffer[ESC_BUFFER_ITEMS];
    /// next throttle
    double nextThrottle = 0;

    /// isr
    static void isr(void* arg);

    /// the maximum throttle for all ESCs
    static uint16_t maxThrottle;
public:
    /**
     * @brief registers a new ESC
     * 
     * @param telemetryStream stream for telemetry acquisition
     * @param signalPin pin for data output (DShot)
     * @param telemetryPin pin for telemetry reception (Baud 115200)
     * @param dmaChannelTX dmaChannel for sending the packets
     * @param dmaChannelRX dma channel for receiving erpm updates
     * @param onError callback method when an error occurs, takes an ESC pointer and an error code
     * @param onStatusChange callback method when the status changes (fired at loop()), takes an ESC pointer, the old and the new status
     */
    ESC(/*HardwareSerial *telemetryStream, */int8_t signalPin, int8_t telemetryPin, rmt_channel_t dmaChannelTX, rmt_channel_t dmaChannelRX, void (*onError) (ESC *esc, uint8_t errorCode), void (*onStatusChange) (ESC *esc, uint8_t newStatus, uint8_t oldStatus));
    /// @brief destroys the ESC object
    ~ESC();
    /**
     * @brief sets the new ERPM value from telemetry after RPS conversion
     * 
     * @param newERPM the new erpm value
     */
    void setERPM(uint32_t newERPM);
    /** @brief checks for telemetry
     * 
     * reads errors (onError), supplies telemetry to telemetry variables, checks if ESC is connected, checks for status change (onStatusChange)
     * @return true if new telemetry
     */
    // bool loop();
    /// @brief disables ESC temporarily
    void pause();
    /// @brief enables ESC after it was paused
    void resume();
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
    void setThrottle(double newThrottle);
    /**
     * @brief This method will write the newThrottle to currentThrottle and send the newThrottle to the ESC
     * 
     * If manual data is present, it will send manualData instead, until it is empty<br>implements safety measures like cutoffVoltage
     */
    void IRAM_ATTR send();
    /**
     * @brief sends a throttle value immediately, no checks, no telemetry request
     * 
     * DO NOT USE THIS FOR NORMAL OPERATION. USE send() and setThrottle() INSTEAD.
     * @param throttleValue the throttle value (0...2000)
     */
    void IRAM_ATTR sendThrottle(uint16_t throttleValue);
    /**
     * @brief sends an 11 bit raw value immediately, no checks, but telemetry request
     * 
     * DO NOT USE THIS FOR NORMAL OPERATION. USE send() and setThrottle()/manualData INSTEAD.
     * @param rawValueWithoutChecksum raw 11 bit value (right aligned)
     */
    void IRAM_ATTR sendRaw11(uint16_t rawValueWithoutChecksum);
    /**
     * @brief sends a 16 bit raw value immediately, no checks, telemetry bit is set by user
     * 
     * DO NOT USE THIS FOR NORMAL OPERATION. USE send() and setThrottle()/manualData INSTEAD, UNLESS TELEMETRY BIT DOESN'T ALLOW.
     * @param rawValueWithChecksum raw 16 bit value
     */
    void IRAM_ATTR sendFullRaw(uint16_t rawValueWithChecksum);
    /**
     * @brief check if red LED is on
     * 
     * @return true (1) if LED is on
     */
    bool getRedLED();
    /**
     * @brief check if green LED is on
     * 
     * @return true (1) if LED is on
     */
    bool getGreenLED();
    /**
     * @brief check if blue LED is on
     * 
     * @return true (1) if LED is on
     */
    bool getBlueLED();
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
    /// voltage as supplied by telemetry, in cV
    uint16_t voltage = 0;
    /// eRPM of the motor (divide by 60*(motorpoles/2))
    uint16_t eRPM = 0;
    /// speed (calculated by using wheel diameter and pole count, not implemented yet)
    uint16_t speed = 0;
    /// temperature as supplied by telemetry
    uint8_t temperature = 0;
    /// status byte (read using masks)
    uint8_t status = DEFAULT_STATUS;
    /// manual data array, put data here and set manualDataAmount
    uint16_t manualData11[MAX_MANUAL_DATA];
    /// last useful index of manualData11 +1, so for 3 manual data packets, write 3 here
    uint16_t manualDataAmount = 0;
    ///currentThrottle (read only)
    double currentThrottle = 0;
    /// voltage at which all ESCs will fire a VOLTAGE_LOW error
    static uint16_t cutoffVoltage;
    /// dma channel used for transmission
    rmt_channel_t dmaChannelTX = (rmt_channel_t)0;
    /// dma channel used for bidirectional dshot
    rmt_channel_t dmaChannelRX = (rmt_channel_t)0;
};

#endif