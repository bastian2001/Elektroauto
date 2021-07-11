#include "global.h"
#include "wifiStuff.h"
#include "system.h"
#include "messageHandler.h"

uint32_t nextCheck = 0;
uint8_t warningVoltageCount = 0;

void setArmed (bool arm){
  if (arm != armed){
    if (!raceActive){
      reqValue = 0;
      targetERPM = 0;
    }
    broadcastWSMessage((raceActive ? arm : arm || raceMode) ? "UNBLOCK VALUE" : "BLOCK VALUE 0");
    armed = arm;
    setThrottle(0);
    nextThrottle = 0;
  }
}

void IRAM_ATTR setThrottle(double newThrottle) {
  newThrottle = (newThrottle > 2000) ? 2000 : newThrottle;
  newThrottle = (newThrottle < 0) ? 0 : newThrottle;
  newThrottle = (newThrottle > maxThrottle) ? maxThrottle : newThrottle;
  throttle = newThrottle;
  newThrottle += (newThrottle == 0) ? 0 : 47;
  escValue = appendChecksum(newThrottle);
}

uint16_t IRAM_ATTR appendChecksum(uint16_t value, bool telemetryRequest) {
  value &= 0x7FF;
  value = (value << 1) | telemetryRequest;
  int csum = 0, csum_data = value;
  for (int i = 0; i < 3; i++) {
    csum ^=  csum_data;
    csum_data >>= 4;
  }
  csum &= 0xf;
  value = (value << 4) | csum;
  return value;
}

void startRace(){
  if (!raceActive && raceMode){
    broadcastWSMessage("BLOCK RACEMODETOGGLE ON");
    raceActive = true;
    setArmed(true);
    setNewTargetValue();
  } else if (!raceMode) {
    broadcastWSMessage("SET RACEMODETOGGLE OFF");
  }
}

double calcThrottle(int target, int was[], double masterMultiplier) {
  double was_avg = 0;
  int was_sum = 0, t_sq_sum = 0, t_multi_was_sum = 0;
  for (int i = 0; i < TREND_AMOUNT; i++) {
    int t = i - TREND_AMOUNT / 2;
    was_sum += was[i];
    t_sq_sum += t*t;
    t_multi_was_sum += t * was[i];
  }
  was_avg = (double)was_sum / TREND_AMOUNT;

  double m = (double)t_multi_was_sum / (double)t_sq_sum;

  double prediction = m * ((double)TREND_AMOUNT - (double)(TREND_AMOUNT / 2)) + (double)was_avg;
  if (prediction < 0) prediction = 0;
  double deltaERPM = target - prediction;
  double delta_throttle = erpmA * pow(deltaERPM, 3) + erpmB * pow(deltaERPM, 2) + erpmC * deltaERPM;
  delta_throttle *= pidMulti * masterMultiplier;

  return throttle + delta_throttle;
}

void receiveSerial() {
  if (Serial.available()) {
    String readout = Serial.readStringUntil('\n');
    processMessage(readout, 255);
  }
}

float rpsToErpm(float rps){
  return (rps / rpsConversionFactor + .5f);
}

float erpmToRps(float erpm){
  return (erpm * rpsConversionFactor + .5f);
}

void setNewTargetValue(){
  switch (ctrlMode) {
    case MODE_THROTTLE:
      nextThrottle = reqValue;
      break;
    case MODE_RPS:
      if (reqValue > maxTargetRPS)
        reqValue = maxTargetRPS;
      targetERPM = rpsToErpm(reqValue);
      break;
    case MODE_SLIP:
      if (reqValue > maxTargetSlip)
        reqValue = maxTargetSlip;
      targetSlip = reqValue;
      break;
  }
}

void sendRaceLog(){
  raceModeSendValues = false;
  delay(2);
  broadcastWSBin((uint8_t*)logData, LOG_FRAMES * 9, true, 20);
}

void throttleRoutine(){
  if (armed) {
    switch (ctrlMode) {
      int addToTargetERPM;
      case MODE_THROTTLE:
        break;
      case MODE_RPS:
        nextThrottle = calcThrottle(targetERPM, previousERPM);
        break;
      case MODE_SLIP:
        addToTargetERPM = zeroERPMOffset - ((float)throttle * (float) zeroERPMOffset / (float) zeroOffsetAtThrottle);
        if (addToTargetERPM < 0)
          addToTargetERPM = 0;
        targetERPM = ((-(float)speedBMI) / ((float) targetSlip * .01f - 1)) / erpmToMMPerSecond + addToTargetERPM;
        nextThrottle = calcThrottle(targetERPM, previousERPM, .1);
        break;
    }
  }
}

void checkVoltage(){
  if (millis() > nextCheck){
    nextCheck += 10000;
    if (telemetryVoltage < warningVoltage && telemetryVoltage != 0 && telemetryVoltage != 257){
      warningVoltageCount++;
      if (warningVoltageCount % 5 == 3){
        broadcastWSMessage(F("MESSAGEBEEP Warnung! Spannung niedrig!"));
        manualData[0] = 0xbb;
        manualDataAmount = 1;
      }
    } else {
      warningVoltageCount = 0;
    }
  }
}

uint16_t getMaxValue (int mode){
  switch (mode){
    case MODE_THROTTLE:
      return maxThrottle;
    case MODE_RPS:
      return maxTargetRPS;
    case MODE_SLIP:
      return maxTargetSlip;
    default:
      return 0;
  }
}