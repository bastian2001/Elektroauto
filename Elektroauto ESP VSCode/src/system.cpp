#include "global.h"
#include "WiFiHandler.h"
#include "system.h"
#include "messageHandler.h"
#include "settings.h"
#include "LED.h"

uint32_t nextCheck = 0;
uint8_t warningVoltageCount = 0;

void IRAM_ATTR setArmed (bool arm){
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

void setMode(uint8_t mode){
  ctrlMode = mode;
  String modeText = "SET MODESPINNER ";
  modeText += ctrlMode;
  switch(ctrlMode){
    case MODE_THROTTLE:
      reqValue = (((int)(ESCs[0]->currentThrottle)) + ((int)(ESCs[1]->currentThrottle))) / 2; // stationary
      break;
    case MODE_RPS:
      reqValue = erpmToRps((((uint16_t)(ESCs[0]->eRPM)) + ((uint16_t)(ESCs[1]->eRPM))) / 2);
      break;
    case MODE_SLIP:
      reqValue = 0;
      break;
  }
  setNewTargetValue();
  broadcastWSMessage(modeText);
  broadcastWSMessage(String("MAXVALUE ") + String(getMaxValue(ctrlMode)));
}

void IRAM_ATTR startRace(){
  if (!raceActive && raceMode){
    broadcastWSMessage("BLOCK RACEMODETOGGLE ON");
    raceActive = true;
    setArmed(true);
    setNewTargetValue();
    setStatusLED(LED_RACE_ARMED_ACTIVE);
  } else if (!raceMode) {
    broadcastWSMessage("SET RACEMODETOGGLE OFF");
  }
}

double calcThrottle(int target, int was[], double currentThrottle, double masterMultiplier) {
  // double was_avg = 0;
  // int was_sum = 0, t_sq_sum = 0, t_multi_was_sum = 0;
  // for (int i = 0; i < TREND_AMOUNT; i++) {
  //   int t = i - TREND_AMOUNT / 2;
  //   was_sum += was[i];
  //   t_sq_sum += t*t;
  //   t_multi_was_sum += t * was[i];
  // }
  // was_avg = (double)was_sum / TREND_AMOUNT;

  // double m = (double)t_multi_was_sum / (double)t_sq_sum;

  // double prediction = m * ((double)TREND_AMOUNT - (double)(TREND_AMOUNT / 2)) + (double)was_avg;
  // if (prediction < 0) prediction = 0;
  // double deltaERPM = target - prediction;
  // double delta_throttle = erpmA * deltaERPM*deltaERPM*deltaERPM + erpmB * deltaERPM*deltaERPM + erpmC * deltaERPM;
  // delta_throttle *= pidMulti * masterMultiplier;

  // return currentThrottle + delta_throttle;

  // proportional
  double proportional = (target - was[TREND_AMOUNT - 1]) * kP;

  // integral
  double integral = kI * (double) integ;

  // derivative
  double derivative = (was[TREND_AMOUNT - 1] - was[0]) * kD;


  return proportional + integral + derivative;
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
    Action action = actionQueue[i];
    switch (actionQueue[i].type){
      case 0:
        goto exitRunActionsLoop;
      case 1:
        if (action.time == 0 || action.time < millis()){
          setArmed(action.payload);

          for (int x = i; x < 49; x++){
            actionQueue[x] = actionQueue[x+1];
          }
          actionQueue[49] = Action();
        } else {
          i++;
        }
        break;
      case 2:
        if (action.time == 0 || action.time < millis()){
          broadcastWSMessage(String((char*)action.payload), false, 5, true);
          free((void*)action.payload);

          for (int x = i; x < 49; x++){
            actionQueue[x] = actionQueue[x+1];
          }
          actionQueue[49] = Action();
        } else {
          i++;
        }
        break;
      case 3:
        if (action.time == 0 || action.time < millis()){
          setMode(action.payload);

          actionQueue[49] = Action();

          for (int x = i; x < 49; x++){
            actionQueue[x] = actionQueue[x+1];
          }
          actionQueue[49] = Action();
        } else {
          i++;
        }
        break;
      // case 255:
      //   if (action.time == 0 || action.time > millis()){
      //     (*(action.fn))(action.payload, action.payloadLength);
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

void IRAM_ATTR setNewTargetValue(){
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

void enableRaceMode(bool en){
  if (en){
    broadcastWSMessage("SET RACEMODETOGGLE ON");
    broadcastWSMessage("UNBLOCK VALUE");
    setStatusLED(LED_RACE_MODE);
  } else {
    broadcastWSMessage("SET RACEMODETOGGLE OFF");
    if (!((ESCs[0]->status) & ARMED_MASK))
      broadcastWSMessage("BLOCK VALUE 0");
    if (statusLED == LED_RACE_MODE) resetStatusLED();
  }
  raceMode = en;
}