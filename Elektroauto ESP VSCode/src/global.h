//! @file global.h global functions and parameters

#include <Arduino.h>
#include <WebSocketsServer.h>
#include "driver/rmt.h"
#include <EEPROM.h>
#include "WiFi.h"
#include <esp_task_wdt.h>
#include "BMI160Gen.h"
#include "ESC.h"



/// pin used for output to ESC1
#define ESC1_OUTPUT_PIN 16
/// pin used for input from ESC1
#define ESC1_INPUT_PIN 21
/// pin used for output to ESC2
#define ESC2_OUTPUT_PIN 25
/// pin used for input from ESC1
#define ESC2_INPUT_PIN 27
/// pin used for the transmission indicator
#define TRANSMISSION_PIN 33
/// pin of the built-in LED
#define LED_BUILTIN 22
/// pin used for MISO of the SPI driver
#define SPI_MISO 19
/// pin used for MOSI of the SPI driver
#define SPI_MOSI 23
/// pin used for CS/SS of the SPI driver
#define SPI_CS 5
/// pin used for SCL/SCK of the SPI driver
#define SPI_SCL 18
/// pin used for LED
#define LED_PIN LED_BUILTIN
/// pin used for button control
#define BUTTON_PIN 4
/// pin used for the phototransistor
#define LIGHT_SENSOR_PIN 32


///frequency of basically everything
#define ESC_FREQ 1600


//ESC/DShot debugging settings
/// transmission indicator every ... frames; 0 for disabled (recommended for final/semi-stable build)
#define TRANSMISSION_IND 5000
/// Print telemetry from ESC out to Serial every ... frames; 0 for disabled (recommended for final/semi-stable build); use lower values with care
#define TELEMETRY_DEBUG 0
/// Print the throttle when printing the telemetry
#define PRINT_TELEMETRY_THROTTLE
/// Print the temperature of the ESC when printing the telemetry
#define PRINT_TELEMETRY_TEMP
/// Print the ERPM when printing the telemetry
#define PRINT_TELEMETRY_ERPM
/// Print the voltage when printing the telemetry
#define PRINT_TELEMETRY_VOLTAGE

//WiFi and WebSockets settings
/// Maximum number of concurrent WebSocket connections
#define MAX_WS_CONNECTIONS 5
/// send out telemetry to clients every ... ms
#define TELEMETRY_UPDATE_MS 20
/// add ... ms to telemetry frequency for every connected client (1 client: Telemetry every TELEMETRY_UPDATE_MS + /TELEMETRY_UPDATE_ADD milliseconds)
#define TELEMETRY_UPDATE_ADD 20
/// The SSID
// #define ssid "Bloedfrauen und -maenner"
/// The password for wifi
// #define password "CaputDraconis"
#define ssid "POCO X3 NFC"
#define password "hallo123"
// #define ssid "springernet"
// #define password "CL7B1L609235"

//debugging settings
/// print setup info (e.g. IP-Address)
#define PRINT_SETUP
/// print info about WebSocket connections, e.g. new connections
#define PRINT_WEBSOCKET_CONNECTIONS
/// print incoming messages from WebSocket clients
#define PRINT_INCOMING_MESSAGES
/// print broadcasts into the Serial connection
#define PRINT_BROADCASTS

//PID loop settings
/// number of frames for RPS calculation, to be tested
#define TREND_AMOUNT 20

//logging settings
/// number of frames that are logged in race mode
#define LOG_FRAMES ESC_FREQ * 4
/// logging frequency divider, e.g. ESC_FREQ is 3200Hz, then a divider of 2 will log at 1600Hz
#define LOG_FREQ_DIVIDER 1
/// number of bytes needed per log frame
#define BYTES_PER_LOG_FRAME 6
/// bytes reserved for logging
#define LOG_SIZE (LOG_FRAMES * BYTES_PER_LOG_FRAME / LOG_FREQ_DIVIDER + 256)

/// defines modes
enum Modes {
    MODE_THROTTLE = 0,
    MODE_RPS,
    MODE_SLIP
};

/// defines LED blinking behaviour
enum LEDModes {
    LED_OFF = 0,
    LED_SETUP,
    LED_NO_DEVICE,
    LED_RACE_MODE,
    LED_FOUND_BLOCK,
    LED_RACE_ARMED_ACTIVE,
    LED_HALF_RESET,
    LED_HALF_RESET_PRESSED,
    LED_HALF_RESET_DANGER,
};
// the status represents a hirarchy, call resetStatusLED() to reset it to 0

typedef struct logFrame{
    uint16_t throttle0;
    uint16_t throttle1;
    uint16_t erpm0;
    uint16_t erpm1;
    uint16_t pTerm0;
    uint16_t iTerm0;
    uint16_t dTerm0;
    uint16_t i2Term0;
    int16_t acceleration;
    uint16_t targetERPM;
} LogFrame;

/// Action for disarm etc
typedef struct Action{
    uint8_t type = 0; //1 = setArmed, 2 = broadcast payload (char array) to all WS clients, 3 = set mode, 255 = own function
    // void * fn;
    int payload = 0; // payload or (int casted) payload pointer (for own function)
    size_t payloadLength = 0; //payload length
    unsigned long time = 0; //millis at which the action will be triggered, 0 for immediately
} action;

///defines the types of button events
enum class ButtonEventType{
    ShortPress = 0,
    LongPress,
    DoublePress
};
/// A button event
typedef struct buttonEvent{
    ButtonEventType type = ButtonEventType::ShortPress;
    unsigned long time = 0;
} ButtonEvent;

/// PII²D log part
typedef struct pii2dLogHelper{
    uint16_t proportional = 0;
    uint16_t integral = 0;
    uint16_t derivative = 0;
    uint16_t iSquared = 0;
} ControlLogFrame;

/// stores the last button event (e.g. for double press actions)
extern ButtonEvent lastButtonEvent;
/// when the button was last pressed
extern unsigned long lastButtonDown;

/// status LED
extern int statusLED;


// rps control variables
/// holds the master multiplier for RPS/slip control, default value is set here
extern double pidMulti;
/// additional multiplier for slip control
extern double slipMulti;
/// große Änderungen: zu viel -> overshooting bei großen Anpassungen, abwürgen. Zu wenig -> langsames Anpassen bei großen Änderungen; holds the third power value for RPS/slip control, default value is given here
extern double erpmA;
/// holds the second power value for RPS/slip control, default value is set here
extern double erpmB;
/// responsiveness: zu viel -> schnelles wackeln um den eigenen Wert, evtl. overshooting bei kleinen Anpassungen. zu wenig -> langsames Anpassen bei kleinen Änderungen; holds the linear factor for RPS/slip control, default value is set here
extern double erpmC;
/// holds i term
extern int32_t integ;
/// P gain for PID controller
extern double kP;
/// I gain for PID controller
extern double kI;
/// D gain for PID controller
extern double kD;
/// I² gain for PII²D controller
extern double kI2;



// motor and wheel settings
/// holds the maximum allowed target RPS, default value is set here
extern uint16_t maxTargetRPS;
/// holds the maximum allowed target slip ratio, default value is set here
extern uint8_t maxTargetSlip;
/// holds the count of motor poles, default value is set here
extern uint8_t motorPoleCount;
/// holds the wheel diameter in mm, default value is set here
extern uint8_t wheelDiameter;
// don't change anything here, this makes conversions from rps to erpm and back easier
extern double rpsConversionFactor;
// don't change anything here, this makes conversions from erpm to mm/s and back easier
extern float erpmToMMPerSecond;
/// slip control: throttle at zero ERPM / starting throttle, default value is set here
extern uint16_t zeroERPMOffset;
/// slip control: point at which no more throttle is given than calculated, default value is set here
extern uint16_t zeroOffsetAtThrottle;

// EEPROM
/// don't change anything here, keeps track of whether/when to save the settings to EEPROM
extern bool commitFlag;

// voltage settings
/// holds the value for the warning voltage (user is warned of low voltage), default value is set here
extern uint16_t warningVoltage;

// control variables
extern int targetERPM, ctrlMode, reqValue, targetSlip;

// throttle calculation
extern int previousERPM[2][TREND_AMOUNT];

// race mode
extern bool raceModeSendValues, raceMode, raceActive;
extern uint8_t *logData;
extern uint16_t *throttle_log0, *throttle_log1, *erpm_log0, *erpm_log1;
extern uint16_t *p_term_log0, *p_term_log1, *i_term_log0, *i_term_log1, *i2_term_log0, *i2_term_log1, *d_term_log0, *d_term_log1;
extern int16_t *acceleration_log;
extern int16_t *bmi_temp_log;
extern uint32_t *target_erpm_log;
extern uint16_t logPosition;
extern uint32_t raceStartedAt;
extern uint16_t finishFrame;
extern ControlLogFrame pidLoggers[2];

// accelerometer
extern double distBMI, speedBMI, acceleration;
extern int16_t rawAccel, bmiRawTemp;
extern int8_t bmiTemp;
extern bool calibrateFlag;

// WiFi/WebSockets
extern WebSocketsServer webSocket;
extern uint8_t clients[MAX_WS_CONNECTIONS][2]; //[device (disconnected, app, web)][telemetry (off, on)]
extern uint8_t telemetryClientsCounter;

// Action queue
extern Action actionQueue[50];

// ESCs
extern ESC *ESCs[2]; //0: left, 1: right

// error
extern uint16_t errorCount;

// light sensor
extern int * lsStatePtr;


/** 
 * @brief add anything to the Serial string
 * @param s the String to print
 */
void sPrint(String s);

/** 
 * @brief add a line to the Serial string
 * @param s the String to print
 */
void sPrintln(String s);

void printSerial();