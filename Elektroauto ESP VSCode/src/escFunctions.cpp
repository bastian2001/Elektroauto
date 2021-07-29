#include "global.h"
#include "system.h"
#include "accelerometerFunctions.h"

uint8_t currentESC = 0;

void IRAM_ATTR escIR() {
  // #if TRANSMISSION_IND != 0
  // escOutputCounter = (escOutputCounter == TRANSMISSION_IND - 1) ? 0 : escOutputCounter + 1;
  // if (escOutputCounter == 0)
  //   digitalWrite(TRANSMISSION_PIN, LOW);
  // delayMicroseconds(20);
  // #endif

  ESCs[currentESC]->send();
  currentESC = 1 - currentESC;
  
  // #if TRANSMISSION_IND != 0
  // if (escOutputCounter == 0)
  //   digitalWrite(TRANSMISSION_PIN, HIGH);
  // #endif

  // record new previousERPM value
  for (int i = 0; i < TREND_AMOUNT - 1; i++) {
    previousERPM[i] = previousERPM[i + 1];
  }
  previousERPM[TREND_AMOUNT - 1] = ESCs[0]->heRPM;

  // handle BMI routine
  readBMI();

  // print debug telemetry over Serial
  // #if TELEMETRY_DEBUG != 0
  // escOutputCounter3 = (escOutputCounter3 == TELEMETRY_DEBUG - 1) ? 0 : escOutputCounter3 + 1;
  // if (escOutputCounter3 == 0){ 
  //   #ifdef PRINT_TELEMETRY_THROTTLE
  //     sPrint((String)((int)throttle));
  //     sPrint("\t");
  //   #endif
  //   #ifdef PRINT_TELEMETRY_TEMP
  //     sPrint((String)telemetryTemp);
  //     sPrint("\t");
  //   #endif
  //   #ifdef PRINT_TELEMETRY_VOLTAGE
  //     sPrint((String)telemetryVoltage);
  //     sPrint("\t");
  //   #endif
  //   #ifdef PRINT_TELEMETRY_ERPM
  //     sPrint((String)telemetryERPM);
  //     sPrint("\t");
  //   #endif
  //   #if defined(PRINT_TELEMETRY_THROTTLE) || defined(PRINT_TELEMETRY_TEMP) || defined(PRINT_TELEMETRY_VOLTAGE) || defined(PRINT_TELEMETRY_ERPM)
  //     sPrintln("");
  //   #endif
  // }
  // #endif

  // logging, if race is active
  if (raceActive){
    throttle_log[logPosition] = ESCs[0]->currentThrottle;
    acceleration_log[logPosition] = 0;
    erpm_log[logPosition] = previousERPM[TREND_AMOUNT - 1];
    voltage_log[logPosition] = ESCs[0]->voltage;
    temp_log[logPosition] = ESCs[0]->temperature;
    logPosition++;
    if (logPosition == LOG_FRAMES){
      raceActive = false;
      raceModeSendValues = true;
    }
  }
}