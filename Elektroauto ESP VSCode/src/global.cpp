#include "global.h"

// rps control settings
double pidMulti = 1;
double erpmA = 0.000000008;
double erpmB = 0.0000006;
double erpmC = 0.01;

// motor and wheel settings
uint16_t maxThrottle = 2000;
uint16_t maxTargetRPS = 1500;
uint8_t maxTargetSlip = 20;
uint8_t motorPoleCount = 12;
uint8_t wheelDiameter = 30;
float rpsConversionFactor = (1.6667f / ((float)motorPoleCount / 2.0f));
float erpmToMMPerSecond = (rpsConversionFactor * (float)wheelDiameter * PI);
uint16_t zeroERPMOffset = 40;
uint16_t zeroOffsetAtThrottle = 200;

// EEPROM
bool commitFlag = false;

// voltage settings
uint16_t cutoffVoltage = 640;
uint16_t warningVoltage = 740;

// control variables
int targetERPM = 0, ctrlMode = MODE_THROTTLE, reqValue = 0, targetSlip = 0;
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
uint16_t manualData[MAX_MANUAL_DATA];

// throttle calculation
int previousERPM[TREND_AMOUNT];

// race mode, adjust LOG_SIZE when changing logged data
bool raceMode = false, raceActive = false, raceModeSendValues = false;
uint16_t *logData;
uint16_t *throttle_log, *erpm_log, *voltage_log;
int16_t *acceleration_log;
uint8_t *temp_log;
uint16_t logPosition = 0;

// accelerometer
double distBMI = 0, speedBMI = 0, acceleration = 0;
int16_t rawAccel = 0;

// WiFi/WebSockets
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