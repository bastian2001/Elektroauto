#include <Arduino.h>
#include "global.h"
#include "system.h"
#include "escSetup.h"
#include "mpuFunctions.h"

int escOutputCounter = 0, escOutputCounter3 = 0;
int previousERPM[TREND_AMOUNT];
extern uint16_t telemetryERPM, telemetryVoltage;
extern uint8_t telemetryTemp;
extern bool raceModeSendValues;
bool armed = false, raceActive = false;
extern uint16_t escValue;
uint16_t logPosition = 0;
extern uint16_t throttleLog[LOG_FRAMES], erpmLog[LOG_FRAMES], voltageLog[LOG_FRAMES];
extern int accelerationLog[LOG_FRAMES];
extern uint8_t tempLog[LOG_FRAMES];
double throttle = 0, nextThrottle = 0;
extern bool updatedValue;
volatile bool escirFinished = false;
// portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR escir() {
  while (escirFinished){}
  //set Throttle to evaluated value
  if (armed){
    setThrottle(nextThrottle);
  }

  #ifdef SEND_TRANSMISSION_IND
  escOutputCounter = (escOutputCounter == TRANSMISSION_IND) ? 0 : escOutputCounter + 1;
  if (escOutputCounter == 0)
    digitalWrite(TRANSMISSION, LOW);
  delayMicroseconds(20);
  #endif

  //send value to ESC
  esc_send_value(escValue, false);
  #ifdef SEND_TRANSMISSION_IND
  if (escOutputCounter == 0)
    digitalWrite(TRANSMISSION, HIGH);
  #endif

  // int now = micros() % 1000;
  // if (now > 20)
  // sPrintln(String(now % 1000));

  // record new previousERPM value
  for (int i = 0; i < TREND_AMOUNT - 1; i++) {
    previousERPM[i] = previousERPM[i + 1];
  }
  previousERPM[TREND_AMOUNT - 1] = telemetryERPM;

  #if TELEMETRY_DEBUG > -1
  // print debug telemetry over Serial
  escOutputCounter3 = (escOutputCounter3 == TELEMETRY_DEBUG) ? 0 : escOutputCounter3 + 1;
  if (escOutputCounter3 == 0){ 
    #ifdef PRINT_TELEMETRY_THROTTLE
      sPrint((String)((int)throttle));
      sPrint("\t");
    #endif
    #ifdef PRINT_TELEMETRY_TEMP
      sPrint((String)telemetryTemp);
      sPrint("\t");
    #endif
    #ifdef PRINT_TELEMETRY_VOLTAGE
      sPrint((String)telemetryVoltage);
      sPrint("\t");
    #endif
    #ifdef PRINT_TELEMETRY_ERPM
      sPrint((String)telemetryERPM);
      sPrint("\t");
    #endif
    #if defined(PRINT_TELEMETRY_THROTTLE) || defined(PRINT_TELEMETRY_TEMP) || defined(PRINT_TELEMETRY_VOLTAGE) || defined(PRINT_TELEMETRY_ERPM)
      sPrintln("");
    #endif
  }
  #endif

  // logging, if race is active
  if (raceActive){
    throttleLog[logPosition] = (uint16_t)throttle;
    accelerationLog[logPosition] = raw_accel;
    erpmLog[logPosition] = previousERPM[TREND_AMOUNT - 1];
    voltageLog[logPosition] = telemetryVoltage;
    tempLog[logPosition] = telemetryTemp;
    logPosition++;
    if (logPosition == LOG_FRAMES){
      raceActive = false;
      raceModeSendValues = true;
    }
  }
  escirFinished = true;
}