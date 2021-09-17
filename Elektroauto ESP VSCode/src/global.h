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
#define ESC1_OUTPUT_PIN 25
/// pin used for input from ESC1
#define ESC1_INPUT_PIN 21
/// pin used for output to ESC2
#define ESC2_OUTPUT_PIN 27
/// pin used for input from ESC1
#define ESC2_INPUT_PIN 16
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


///frequency of basically everything
#define ESC_FREQ 800


//ESC/DShot debugging settings
/// transmission indicator every ... frames; 0 for disabled (recommended for final/semi-stable build)
#define TRANSMISSION_IND 0
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
// #define ssid "Test"
/// The password for wifi
// #define password "CaputDraconis"
#define ssid "POCO X3 NFC"
#define password "055fb39a4cc4"
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
#define TREND_AMOUNT 5 //nur ungerade!! 

//logging settings
/// number of frames that are logged in race mode
#define LOG_FRAMES 3000
/// number of bytes needed per log frame
#define BYTES_PER_LOG_FRAME 18
/// bytes reserved for logging
#define LOG_SIZE (LOG_FRAMES * BYTES_PER_LOG_FRAME)

/// defines modes
enum Modes {
    MODE_THROTTLE = 0,
    MODE_RPS,
    MODE_SLIP
};

/// Action for disarm etc
typedef struct Action{
    uint8_t type = 0; //1 = setArmed, 2 = broadcast payload (char array) to all WS clients, 255 = own function
    // void * fn;
    int payload = 0; // payload or (int casted) payload pointer (for own function)
    size_t payloadLength = 0; //payload length
    unsigned long time = 0; //millis at which the action will be triggered, 0 for immediately
} action;

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
extern float rpsConversionFactor;
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
extern uint16_t *logData;
extern uint16_t *throttle_log[2], *erpm_log[2], *voltage_log[2];
extern uint8_t *temp_log[2];
extern int16_t *acceleration_log;
extern int16_t *bmi_temp_log;
extern uint16_t logPosition;

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