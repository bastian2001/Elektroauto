#include "global.h"
#include "wifiStuff.h"
#include "system.h"
#include "messageHandler.h"

uint32_t nextCheck = 0;
uint8_t warningVoltageCount = 0;

void setArmed (bool arm){
  if (arm != ((ESCs[0]->status & ARMED_MASK) > 0)){
    if (!raceActive){
      reqValue = 0;
      targetERPM = 0;
    }
    broadcastWSMessage((raceActive ? arm : (arm || raceMode)) ? "UNBLOCK VALUE" : "BLOCK VALUE 0");
    ESCs[0]->arm(arm);
    ESCs[1]->arm(arm);
  }
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

double calcThrottle(int target, int was[], double currentThrottle, double masterMultiplier) {
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
  double delta_throttle = erpmA * deltaERPM*deltaERPM*deltaERPM + erpmB * deltaERPM*deltaERPM + erpmC * deltaERPM;
  delta_throttle *= pidMulti * masterMultiplier;

  return currentThrottle + delta_throttle;
}

void receiveSerial() {
  delay(1);
  if (Serial.available()) {
    String readout = Serial.readStringUntil('\n');
    processMessage(readout, 255);
  }
}

void runActions() {
  for (uint8_t i = 0; i < 50;){
    switch (actionQueue[i].type){
      case 0:
        goto exitRunActionsLoop;
      case 1:
        if (actionQueue[i].time == 0 || actionQueue[i].time > millis()){
          setArmed(actionQueue[i].payload);
          memmove(&(actionQueue[i]), &(actionQueue[i]) + sizeof(Action), (49 - i) * sizeof(Action));
          actionQueue[49] = Action();
        } else {
          i++;
        }
        break;
      case 2:
        if (actionQueue[i].time == 0 || actionQueue[i].time > millis()){
          Serial.println(actionQueue[i].payload);
          broadcastWSMessage(String((char*)actionQueue[i].payload), false, 5, true);
          free((void*)actionQueue[i].payload);
          memmove(&(actionQueue[i]), &(actionQueue[i]) + sizeof(Action), (49 - i) * sizeof(Action));
          actionQueue[49] = Action();
        } else {
          i++;
        }
        break;
      // case 255:
      //   if (actionQueue[i].time == 0 || actionQueue[i].time > millis()){
      //     (*(actionQueue[i].fn))(actionQueue[i].payload, actionQueue[i].payloadLength);
      //   } else {
      //     i++;
      //   }
      //   break;
    }
  }
  exitRunActionsLoop:;
  return;
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
      ESCs[0]->setThrottle(reqValue);
      ESCs[1]->setThrottle(reqValue);
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
  broadcastWSBin((uint8_t*)logData, LOG_SIZE, true, 20);
}

void throttleRoutine(){
  if ((ESCs[0]->status) & ARMED_MASK) {
    switch (ctrlMode) {
      int addToTargetERPMLeft, addToTargetERPMRight;
      case MODE_THROTTLE:
        break;
      case MODE_RPS:
        ESCs[0]->setThrottle(calcThrottle(targetERPM, previousERPM[0], ESCs[0]->currentThrottle));
        ESCs[1]->setThrottle(calcThrottle(targetERPM, previousERPM[1], ESCs[1]->currentThrottle));
        break;
      case MODE_SLIP:
        addToTargetERPMLeft = zeroERPMOffset - ((float)(ESCs[0]->currentThrottle) * (float) zeroERPMOffset / (float) zeroOffsetAtThrottle);
        if (addToTargetERPMLeft < 0)
          addToTargetERPMLeft = 0;
        addToTargetERPMRight = zeroERPMOffset - ((float)(ESCs[0]->currentThrottle) * (float) zeroERPMOffset / (float) zeroOffsetAtThrottle);
        if (addToTargetERPMRight < 0)
          addToTargetERPMRight = 0;
        
        targetERPM = ((-(float)speedBMI) / ((float) targetSlip * .01f - 1)) / erpmToMMPerSecond + (addToTargetERPMLeft + addToTargetERPMRight) / 2;
        ESCs[0]->setThrottle(calcThrottle(targetERPM, previousERPM[0], ESCs[0]->currentThrottle, slipMulti));
        ESCs[1]->setThrottle(calcThrottle(targetERPM, previousERPM[1], ESCs[1]->currentThrottle, slipMulti));
        break;
    }
  }
}

void checkVoltage(){
  if (millis() > nextCheck){
    nextCheck += 10000;
    if (ESCs[0]->voltage < warningVoltage && ESCs[0]->voltage != 0 && ESCs[0]->status & CONNECTED_MASK){
      warningVoltageCount++;
      if (warningVoltageCount % 5 == 3){
        broadcastWSMessage(F("MESSAGEBEEP Warnung! Spannung niedrig!"));
        ESCs[0]->beep(ESC_BEEP_5);
        ESCs[1]->beep(ESC_BEEP_5);
      }
    } else {
      warningVoltageCount = 0;
    }
  }
}

uint16_t getMaxValue (int mode){
  switch (mode){
    case MODE_THROTTLE:
      return ESC::getMaxThrottle();
    case MODE_RPS:
      return maxTargetRPS;
    case MODE_SLIP:
      return maxTargetSlip;
    default:
      return 0;
  }
}