#include "global.h"
#include "accelerometerFunctions.h"

uint32_t escOutputCounter = 0;

void IRAM_ATTR escIR() {
  #if TRANSMISSION_IND != 0
  escOutputCounter = (escOutputCounter == TRANSMISSION_IND - 1) ? 0 : escOutputCounter + 1;
  if (escOutputCounter == 0)
    digitalWrite(TRANSMISSION_PIN, LOW);
  delayMicroseconds(20);
  #endif

  ESCs[0]->send();
  ESCs[1]->send();
  
  #if TRANSMISSION_IND != 0
  if (escOutputCounter == 0)
    digitalWrite(TRANSMISSION_PIN, HIGH);
  #endif

  // record new previousERPM value
  for (int i = 0; i < TREND_AMOUNT - 1; i++) {
    previousERPM[0][i] = previousERPM[0][i + 1];
    previousERPM[1][i] = previousERPM[1][i + 1];
  }
  previousERPM[0][TREND_AMOUNT - 1] = ESCs[0]->eRPM;
  previousERPM[1][TREND_AMOUNT - 1] = ESCs[1]->eRPM;

  if(ESCs[0]->status & ARMED_MASK)
    integ += targetERPM - ESCs[0]->eRPM;
  else
    integ = 0;

  // handle BMI routine
  readBMI();

  // print debug telemetry over Serial
  #if TELEMETRY_DEBUG != 0
  escOutputCounter3 = (escOutputCounter3 == TELEMETRY_DEBUG - 1) ? 0 : escOutputCounter3 + 1;
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
  static int logFreqHelper = 0;
  if (raceActive && (++logFreqHelper == LOG_FREQ_DIVIDER)){
    logFreqHelper = 0;
    throttle_log0[logPosition] = ESCs[0]->currentThrottle + .5;
    throttle_log1[logPosition] = ESCs[1]->currentThrottle + .5;
    // erpm_log0[logPosition] = ESCs[0]->eRPM;
    // erpm_log1[logPosition] = ESCs[1]->eRPM;
    acceleration_log[logPosition] = rawAccel;
    // bmi_temp_log[logPosition] = bmiRawTemp;
    // p_term_log0[logPosition] = pidLoggers[0].proportional;
    // p_term_log1[logPosition] = pidLoggers[0].proportional;
    // i_term_log0[logPosition] = pidLoggers[0].integral;
    // i_term_log1[logPosition] = pidLoggers[0].integral;
    // d_term_log1[logPosition] = pidLoggers[0].derivative;
    // d_term_log0[logPosition] = pidLoggers[0].derivative;
    // i2_term_log0[logPosition] = pidLoggers[0].iSquared;
    // i2_term_log1[logPosition] = pidLoggers[0].iSquared;
    if (distBMI < 20) finishFrame = logPosition;
    logPosition++;
    if (logPosition == LOG_FRAMES){
      raceActive = false;
      raceModeSendValues = true;
    }
  }
}