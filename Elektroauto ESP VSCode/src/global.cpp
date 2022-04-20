#include "global.h"

//button variables
ButtonEvent lastButtonEvent;
unsigned long lastButtonDown = 0;

// status LED
int statusLED;

// rps control settings
double pidMulti = 1;
double slipMulti = 2;
double erpmA = 0.000000008;
double erpmB = 0.0000006;
double erpmC = 0.01;
int32_t integ = 0;
double kP = .2, kI = .008, kD = 1, kI2 = .05;


// motor and wheel settings
uint16_t maxTargetRPS = 1500;
uint8_t maxTargetSlip = 20;
uint8_t motorPoleCount = 12;
uint8_t wheelDiameter = 30;
double rpsConversionFactor = (1.0f/60.0f / ((float)motorPoleCount / 2.0f));
float erpmToMMPerSecond = (rpsConversionFactor * (float)wheelDiameter * PI);
uint16_t zeroERPMOffset = 40;
uint16_t zeroOffsetAtThrottle = 200;

// EEPROM
bool commitFlag = false;

// voltage settings
uint16_t warningVoltage = 740;

// control variables
int targetERPM = 0, ctrlMode = MODE_THROTTLE, reqValue = 0, targetSlip = 0;

// throttle calculation
int previousERPM[2][TREND_AMOUNT];

// race mode, adjust LOG_SIZE when changing logged data
bool raceMode = false, raceActive = false, raceModeSendValues = false;
uint8_t *logData;
uint16_t *throttle_log0, *throttle_log1, *erpm_log0, *erpm_log1;
uint16_t *p_term_log0, *p_term_log1, *i_term_log0, *i_term_log1, *i2_term_log0, *i2_term_log1, *d_term_log0, *d_term_log1;
int16_t *acceleration_log;
int16_t *bmi_temp_log;
uint32_t *target_erpm_log;
uint16_t logPosition = 0;
uint32_t raceStartedAt = 0;
uint16_t finishFrame = 0;
ControlLogFrame pidLoggers[2];

// accelerometer
double distBMI = 0, speedBMI = 0, acceleration = 0;
int16_t rawAccel = 0, bmiRawTemp = 0;
int8_t bmiTemp = 0;
bool calibrateFlag = false;

// WiFi/WebSockets
WebSocketsServer webSocket = WebSocketsServer(80);
uint8_t clients[MAX_WS_CONNECTIONS][2];
uint8_t telemetryClientsCounter = 0;

// Action queue
Action actionQueue[50];

// ESCs
ESC *ESCs[2];

// error
uint16_t errorCount = 0;

//light sensor
int * lsStatePtr;


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