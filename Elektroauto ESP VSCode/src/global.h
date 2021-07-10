//! @file global.h global functions and parameters

#include <Arduino.h>
#include <WebSocketsServer.h>
#include "driver/rmt.h"
#include <EEPROM.h>
#include "WiFi.h"
#include <esp_task_wdt.h>
#include "BMI160Gen.h"


/*======================================================definitions======================================================*/

//Pin numbers
#define ESC1_OUTPUT_PIN 25 // pin used for output to ESC1
#define ESC1_INPUT_PIN 21 // pin used for input from ESC1
#define ESC2_OUTPUT_PIN 27 // pin used for output to ESC2
#define ESC2_INPUT_PIN 16 // pin used for input from ESC1
#define TRANSMISSION_PIN 33 // pin used for the transmission indicator
#define LED_BUILTIN 22 // pin of the built-in LED
#define SPI_MISO 19 // pin used for MISO of the SPI driver
#define SPI_MOSI 23 // pin used for MOSI of the SPI driver
#define SPI_CS 5 // pin used for CS/SS of the SPI driver
#define SPI_SCL 18 // pin used for SCL/SCK of the SPI driver

//ESC values
#define ESC_FREQ 800 //frequency of basically everything
#define CLK_DIV 3 //DShot 150: 12, DShot 300: 6, DShot 600: 3, change value accordingly for the desired DShot speed
#define TT 44 // total bit time
#define T0H 17 // 0 bit high time
#define T1H 33 // 1 bit high time
#define T0L (TT - T0H) // 0 bit low time
#define T1L (TT - T1H) // 1 bit low time
#define T_RESET 21 // reset length in multiples of bit time
#define ESC_BUFFER_ITEMS 16 // number of bits sent out via DShot


//ESC/DShot debugging settings
#define TRANSMISSION_IND 0 // transmission indicator every ... frames; 0 for disabled (recommended for final/semi-stable build)
#define TELEMETRY_DEBUG 0 // Print telemetry from ESC out to Serial every ... frames; 0 for disabled (recommended for final/semi-stable build); use lower values with care
// #define PRINT_TELEMETRY_THROTTLE
// #define PRINT_TELEMETRY_TEMP
// #define PRINT_TELEMETRY_ERPM
// #define PRINT_TELEMETRY_VOLTAGE

//WiFi and WebSockets settings
#define MAX_WS_CONNECTIONS 5 // Maximum number of concurrent WebSocket connections
#define TELEMETRY_UPDATE_MS 20 // send out telemetry to clients every ... ms
#define TELEMETRY_UPDATE_ADD 20 // add ... ms to telemetry frequency for every connected client (1 client: Telemetry every TELEMETRY_UPDATE_MS + TELEMETRY_UPDATE_ADD milliseconds)
#define ssid "Test" // The SSID
#define password "CaputDraconis" // The password for wifi
// #define ssid "POCO X3 NFC" // The SSID
// #define password "055fb39a4cc4" // The password for wifi
// #define ssid "springernet" // The SSID
// #define password "CL7B1L609235" // The password for wifi

//debugging settings
#define PRINT_SETUP // print setup info (e.g. IP-Address)
#define PRINT_WEBSOCKET_CONNECTIONS // print info about WebSocket connections, e.g. new connections
#define PRINT_INCOMING_MESSAGES // print incoming messages from WebSocket clients
#define PRINT_BROADCASTS // print broadcasts into the Serial connection

//PID loop settings
#define TREND_AMOUNT 5 //nur ungerade!! // number of frames for RPS calculation, to be tested
#define TA_DIV_2 (TREND_AMOUNT / 2)

//logging settings
#define LOG_FRAMES 5000 // number of frames that are logged in race mode
#define BYTES_PER_LOG_FRAME 9
#define LOG_SIZE (LOG_FRAMES * BYTES_PER_LOG_FRAME) // bytes reserved for logging

enum Modes {
    MODE_THROTTLE = 0,
    MODE_RPS,
    MODE_SLIP
};

//rps control variables
extern int targetERPM;
extern int previousERPM[TREND_AMOUNT];
extern double pidMulti;
extern double erpmA; //große Änderungen: zu viel -> overshooting bei großen Anpassungen, abwürgen. Zu wenig -> langsames Anpassen bei großen Änderungen
extern double erpmB;
extern double erpmC; //responsiveness: zu viel -> schnelles wackeln um den eigenen Wert, evtl. overshooting bei kleinen Anpassungen. zu wenig -> langsames Anpassen bei kleinen Änderungen

extern bool commitFlag;

//BMI variables
extern int16_t rawAccel;
extern double distBMI, speedBMI, acceleration;

//slip variables
extern int targetSlip;

//system variables
extern double throttle, nextThrottle;
extern bool armed;
extern int ctrlMode, reqValue;
extern uint16_t escValue;
extern uint16_t cutoffVoltage, warningVoltage;

//motor and wheel settings
extern uint16_t maxThrottle, maxTargetRPS;
extern uint8_t maxTargetSlip, motorPoleCount, wheelDiameter;
extern float rpsConversionFactor, erpmToMMPerSecond;
extern uint16_t zeroERPMOffset;
extern uint16_t zeroOffsetAtThrottle;

// telemetry
extern uint16_t telemetryERPM, telemetryVoltage;
extern uint8_t telemetryTemp;
extern uint16_t speedWheel;

//LED variables
extern bool redLED, greenLED, blueLED;
extern uint8_t newRedLED, newGreenLED, newBlueLED;

//WiFi/WebSockets variables
extern WebSocketsServer webSocket;
extern uint8_t clients[MAX_WS_CONNECTIONS][2]; //[device (disconnected, app, web)][telemetry (off, on)]
extern uint8_t telemetryClientsCounter;

//ESC variables
extern uint16_t telemetryERPM;
extern uint8_t telemetryTemp;
extern uint16_t telemetryVoltage;
extern uint16_t errorCount;
extern uint8_t manualDataAmount;
extern uint16_t manualData[20];

//race mode variables
extern uint16_t *logData;
extern uint16_t *throttle_log, *erpm_log, *voltage_log;
extern int16_t *acceleration_log;
extern uint8_t *temp_log;
extern uint16_t logPosition;
extern bool raceModeSendValues, raceMode, raceActive;

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