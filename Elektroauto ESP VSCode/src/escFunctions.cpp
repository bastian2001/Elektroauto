#include "global.h"
#include "accelerometerFunctions.h"

void IRAM_ATTR escIR() {
  // #if TRANSMISSION_IND != 0
  // escOutputCounter = (escOutputCounter == TRANSMISSION_IND - 1) ? 0 : escOutputCounter + 1;
  // if (escOutputCounter == 0)
  //   digitalWrite(TRANSMISSION_PIN, LOW);
  // delayMicroseconds(20);
  // #endif

  ESCs[0]->send();
  ESCs[1]->send();
  
  // #if TRANSMISSION_IND != 0
  // if (escOutputCounter == 0)
  //   digitalWrite(TRANSMISSION_PIN, HIGH);
  // #endif

  // record new previousERPM value
  for (int i = 0; i < TREND_AMOUNT - 1; i++) {
    previousERPM[0][i] = previousERPM[0][i + 1];
    previousERPM[1][i] = previousERPM[1][i + 1];
  }
  previousERPM[0][TREND_AMOUNT - 1] = ESCs[0]->heRPM;
  previousERPM[1][TREND_AMOUNT - 1] = ESCs[1]->heRPM;

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
    throttle_log[0][logPosition] = ESCs[0]->currentThrottle + .5;
    throttle_log[1][logPosition] = ESCs[1]->currentThrottle + .5;
    erpm_log[0][logPosition] = ESCs[0]->heRPM;
    erpm_log[1][logPosition] = ESCs[1]->heRPM;
    voltage_log[0][logPosition] = ESCs[0]->voltage;
    voltage_log[1][logPosition] = ESCs[1]->voltage;
    temp_log[0][logPosition] = ESCs[0]->temperature;
    temp_log[1][logPosition] = ESCs[1]->temperature;
    acceleration_log[logPosition] = rawAccel;
    bmi_temp_log[logPosition] = bmiRawTemp;
    logPosition++;
    if (logPosition == LOG_FRAMES){
      raceActive = false;
      raceModeSendValues = true;
    }
  }
}