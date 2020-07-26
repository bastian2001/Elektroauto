#include <Arduino.h>
#include "global.h"
#include "system.h"
#include "escSetup.h"

int escOutputCounter = 0, escOutputCounter3 = 0;
int previousERPM[TREND_AMOUNT];
extern uint16_t telemetryERPM, telemetryVoltage;
extern uint8_t telemetryTemp;
extern bool raceModeSendValues;
bool armed = false, raceActive = false;
int targetERPM = 0, ctrlMode = 0, reqValue = 0;
extern uint16_t escValue;
uint16_t logPosition = 0;
extern uint16_t throttle_log[LOG_FRAMES], erpm_log[LOG_FRAMES], voltage_log[LOG_FRAMES];
extern int acceleration_log[LOG_FRAMES];
extern uint8_t temp_log[LOG_FRAMES];
double throttle = 0;

void escir() {
  for (int i = 0; i < TREND_AMOUNT - 1; i++) {
    previousERPM[i] = previousERPM[i + 1];
  }
  previousERPM[TREND_AMOUNT - 1] = telemetryERPM;
  escOutputCounter3 = (escOutputCounter3 == TELEMETRY_DEBUG) ? 0 : escOutputCounter3 + 1;
  if (escOutputCounter3 == 0){ // print debug telemetry over Serial
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

  if (armed) {
    //every esc cycle: calculation of throttle if necessary
    switch (ctrlMode) {
      case 0:
        break;
      case 1:
        setThrottle(calcThrottle(targetERPM, previousERPM));
        break;
      case 2:
        break;
      default:
        break;
    }
  } else {
    setThrottle(0);
  }
  #ifdef SEND_TRANSMISSION_IND
  escOutputCounter = (escOutputCounter == TRANSMISSION_IND) ? 0 : escOutputCounter + 1;
  if (escOutputCounter == 0)
    digitalWrite(TRANSMISSION, LOW);
  delayMicroseconds(20);
  #endif
  esc_send_value(escValue, false);
  #ifdef SEND_TRANSMISSION_IND
  if (escOutputCounter == 0)
    digitalWrite(TRANSMISSION, HIGH);
  #endif
  if (raceActive){
    throttle_log[logPosition] = (uint16_t)(throttle + .5);
    acceleration_log[logPosition] = 0;
    erpm_log[logPosition] = previousERPM[TREND_AMOUNT - 1];
    voltage_log[logPosition] = telemetryVoltage;
    temp_log[logPosition] = telemetryTemp;
    logPosition++;
    if (logPosition == LOG_FRAMES){
      raceActive = false;
      raceModeSendValues = true;
    }
  }
}