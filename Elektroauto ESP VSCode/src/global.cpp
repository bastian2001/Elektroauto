//! @file global.cpp global functions and settings
#include <Arduino.h>
#include <WebSocketsServer.h>
#include "global.h"

double pidMulti = 1;
double erpmA = 0.000000008; //große Änderungen: zu viel -> overshooting bei großen Anpassungen, abwürgen. Zu wenig -> langsames Anpassen bei großen Änderungen
double erpmB = 0.0000006;
double erpmC = 0.01; //responsiveness: zu viel -> schnelles wackeln um den eigenen Wert, evtl. overshooting bei kleinen Anpassungen. zu wenig -> langsames Anpassen bei kleinen Änderungen

uint16_t cutoffVoltage = 640;
uint16_t warningVoltage = 740;

// control variables
int targetERPM = 0, ctrlMode = 0, reqValue = 0, targetSlip = 0;
uint16_t escValue = 0x0011; // telemetry needs to be on before first arm
bool armed = false;
double throttle = 0, nextThrottle = 0;

// telemetry
uint16_t telemetryERPM = 0, telemetryVoltage = 0;
uint8_t telemetryTemp = 0;
uint16_t speedWheel = 0;

// LEDs
bool redLED, greenLED, blueLED;
uint8_t newRedLED, newBlueLED, newGreenLED;

// manual data
uint8_t manualDataAmount = 0;
uint16_t manualData[20];

// throttle calculation
int previousERPM[TREND_AMOUNT];

// race mode
bool raceMode = false, raceActive = false;
uint16_t *logData = 0;
uint16_t *throttle_log, *erpm_log, *voltage_log;
int16_t *acceleration_log;
uint8_t *temp_log;
bool raceModeSendValues = false;
uint16_t logPosition = 0;

// accelerometer variables
int16_t distBMI = 0;
double speedBMI = 0, acceleration = 0;
int16_t rawAccel = 0;

//webSocket
WebSocketsServer webSocket = WebSocketsServer(80);
uint8_t clients[MAX_WS_CONNECTIONS][2];
uint8_t telemetryClientsCounter = 0;

// error
uint16_t errorCount = 0;


String toBePrinted = "";

/** 
 * @brief Print the current Serial string
 */
void printSerial() {
  Serial.print(toBePrinted);
  toBePrinted = "";
}

/** 
 * @brief add anything to the Serial string
 * @param s the String to print
 */
void sPrint(String s) {
  toBePrinted += s;
}

/** 
 * @brief add a line to the Serial string
 * @param s the String to print
 */
void sPrintln(String s) {
  toBePrinted += s;
  toBePrinted += "\n";
}