//! @file global.h global functions and parameters

#include <Arduino.h>
#include <WebSocketsServer.h>
#include "driver/rmt.h"
#include <EEPROM.h>
#include "WiFi.h"
#include <esp_task_wdt.h>
#include "BMI160Gen.h"



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

/// maximum amount of manual data
#define MAX_MANUAL_DATA 20
///frequency of basically everything
#define ESC_FREQ 800
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
#define ssid "Test"
/// The password for wifi
#define password "CaputDraconis"

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
#define LOG_FRAMES 5000
/// number of bytes needed per log frame
#define BYTES_PER_LOG_FRAME 9
/// bytes reserved for logging
#define LOG_SIZE (LOG_FRAMES * BYTES_PER_LOG_FRAME)

/// defines modes
enum Modes {
    MODE_THROTTLE = 0,
    MODE_RPS,
    MODE_SLIP
};

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
/// holds the maximum allowed throttle, default value is set here
extern uint16_t maxThrottle;
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
/// holds the value for the cutoff voltage (no movement possible, car stops), default value is set here
extern uint16_t cutoffVoltage;
/// holds the value for the warning voltage (user is warned of low voltage), default value is set here
extern uint16_t warningVoltage;

// control variables
extern int targetERPM, ctrlMode, reqValue, targetSlip;
extern uint16_t escValue; // telemetry needs to be on before first arm
extern bool armed;
extern double throttle, nextThrottle;

// telemetry
extern uint16_t telemetryERPM, telemetryVoltage;
extern uint8_t telemetryTemp;
extern uint16_t speedWheel;

// LEDs
extern bool redLED, greenLED, blueLED;
extern uint8_t newRedLED, newGreenLED, newBlueLED;

// manual data
extern uint8_t manualDataAmount;
extern uint16_t manualData[MAX_MANUAL_DATA];

// throttle calculation
extern int previousERPM[TREND_AMOUNT];

// race mode
extern bool raceModeSendValues, raceMode, raceActive;
extern uint16_t *logData;
extern uint16_t *throttle_log, *erpm_log, *voltage_log;
extern int16_t *acceleration_log;
extern uint8_t *temp_log;
extern uint16_t logPosition;

// accelerometer
extern double distBMI, speedBMI, acceleration;
extern int16_t rawAccel;
extern bool calibrateFlag;

// WiFi/WebSockets
extern WebSocketsServer webSocket;
extern uint8_t clients[MAX_WS_CONNECTIONS][2]; //[device (disconnected, app, web)][telemetry (off, on)]
extern uint8_t telemetryClientsCounter;

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