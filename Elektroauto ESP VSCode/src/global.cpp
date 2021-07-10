//! @file global.cpp global functions and settings
#include "global.h"

double pidMulti = 1; // holds the master multiplier for RPS/slip control, default value is set here
double erpmA = 0.000000008; //große Änderungen: zu viel -> overshooting bei großen Anpassungen, abwürgen. Zu wenig -> langsames Anpassen bei großen Änderungen; holds the third power value for RPS/slip control, default value is given here
double erpmB = 0.0000006; // holds the second power value for RPS/slip control, default value is set here
double erpmC = 0.01; //responsiveness: zu viel -> schnelles wackeln um den eigenen Wert, evtl. overshooting bei kleinen Anpassungen. zu wenig -> langsames Anpassen bei kleinen Änderungen; holds the linear factor for RPS/slip control, default value is set here

// motor and wheel settings
uint16_t maxThrottle = 2000; // holds the maximum allowed throttle, default value is set here
uint16_t maxTargetRPS = 1500; // holds the maximum allowed target RPS, default value is set here
uint8_t maxTargetSlip = 20; // holds the maximum allowed target slip ratio, default value is set here
uint8_t motorPoleCount = 12; // holds the count of motor poles, default value is set here
uint8_t wheelDiameter = 30; // holds the wheel diameter in mm, default value is set here
float rpsConversionFactor = (1.6667f / ((float)motorPoleCount / 2.0f)); // don't change anything here, this makes conversions from rps to erpm and back easier
float erpmToMMPerSecond = (rpsConversionFactor * (float)wheelDiameter * PI); // don't change anything here, this makes conversions from erpm to mm/s and back easier
uint16_t zeroERPMOffset = 40; // slip control: throttle at zero ERPM / starting throttle, default value is set here
uint16_t zeroOffsetAtThrottle = 200; // slip control: point at which no more throttle is given than calculated, default value is set here

// EEPROM
bool commitFlag = false; //don't change anything here, keeps track of whether/when to save the settings to EEPROM

// voltage settings
uint16_t cutoffVoltage = 640; // holds the value for the cutoff voltage (no movement possible, car stops), default value is set here
uint16_t warningVoltage = 740; // holds the value for the warning voltage (user is warned of low voltage), default value is set here

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
uint16_t manualData[20];

// throttle calculation
int previousERPM[TREND_AMOUNT];

// race mode, adjust LOG_SIZE when changing logged data
bool raceMode = false, raceActive = false;
uint16_t *logData;
uint16_t *throttle_log, *erpm_log, *voltage_log;
int16_t *acceleration_log;
uint8_t *temp_log;
bool raceModeSendValues = false;
uint16_t logPosition = 0;

// accelerometer variables
double distBMI = 0, speedBMI = 0, acceleration = 0;
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