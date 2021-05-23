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
extern uint16_t escValue;
uint16_t logPosition = 0;
extern uint16_t throttle_log[LOG_FRAMES], erpm_log[LOG_FRAMES], voltage_log[LOG_FRAMES];
extern int acceleration_log[LOG_FRAMES];
extern uint8_t temp_log[LOG_FRAMES];
double throttle = 0, nextThrottle = 0;
uint8_t manualDataAmount = 0;
uint16_t manualData[20];

/**
 * @brief the ESC interrupt routine
 * 
 * please do not manually run it, the timer interrupt runs it at the frequency specified in ESC_FREQ
 * runs setThrottle if car is armed, thus applies the checksum
 * if available, it takes the manualData inplace of the escValue and sends the respective value to the esc
 * then, if neccessary, it pushes the manualData array one to the front
 * same thing goes for telemetryERPM history
 * if the race is active, it also logs the data
 */
void escir() {
  //set Throttle
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
  if (manualDataAmount == 0)
    esc_send_value(escValue, false);
  else {
    esc_send_value(manualData[0], false);
    manualDataAmount--;
    for (int i = 0; i < 19; i++){
      manualData[i] = manualData[i+1];
    }
    manualData[19] = 0;
  }
  #ifdef SEND_TRANSMISSION_IND
  if (escOutputCounter == 0)
    digitalWrite(TRANSMISSION, HIGH);
  #endif

  // record new previousERPM value
  for (int i = 0; i < TREND_AMOUNT - 1; i++) {
    previousERPM[i] = previousERPM[i + 1];
  }
  previousERPM[TREND_AMOUNT - 1] = telemetryERPM;

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

  // logging, if race is active
  if (raceActive){
    throttle_log[logPosition] = (uint16_t)throttle;
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