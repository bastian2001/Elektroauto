#include "global.h"
#include "WiFiHandler.h"
#include "system.h"
#include "messageHandler.h"
#include "settings.h"
#include "LED.h"

uint32_t nextCheck = 0;
uint8_t warningVoltageCount = 0;

void setArmed (bool arm){
  if (arm != (ESCs[0]->armed)){
    if (!raceStopAt || !arm){
      reqValue = 0;
    }
    broadcastWSMessage((raceStopAt ? arm : (arm || raceMode)) ? "UNBLOCK VALUE" : "BLOCK VALUE 0");
    ESCs[0]->arm(arm);
    ESCs[1]->arm(arm);
  }
}

void startRace(){
  if (!raceStopAt && raceMode){
    broadcastWSMessage("BLOCK RACEMODETOGGLE ON");
    raceStopAt = millis() + 4000;
    setArmed(true);
    setNewTargetValue();
    setStatusLED(LED_RACE_ARMED_ACTIVE);
    for (int i = 0; i < 2; i++){
      ESCs[i]->manualData11[0] = CMD_LED0_ON;
      ESCs[i]->manualData11[1] = CMD_LED1_ON;
      ESCs[i]->manualData11[2] = CMD_LED2_ON;
      ESCs[i]->manualData11[3] = CMD_LED3_ON;
      ESCs[i]->manualDataAmount = 4;
    }
  } else if (!raceMode) {
    broadcastWSMessage("SET RACEMODETOGGLE OFF");
  }
}

void receiveSerial() {
  delay(1);
  if (Serial.available()) {
    String readout = Serial.readStringUntil('\n');
    processMessage(readout, 255);
  }
}

void setNewTargetValue(){
    ESCs[0]->setThrottle(reqValue);
    ESCs[1]->setThrottle(reqValue);
}

uint16_t getMaxValue (){
  return ESC::getMaxThrottle();
}

void enableRaceMode(bool en){
  if (en){
    broadcastWSMessage("SET RACEMODETOGGLE ON");
    broadcastWSMessage("UNBLOCK VALUE");
    setStatusLED(LED_RACE_MODE);
  } else {
    broadcastWSMessage("SET RACEMODETOGGLE OFF");
    setArmed(false);
    raceStopAt = 0;
    if (statusLED >= LED_RACE_MODE && statusLED <= LED_RACE_ARMED_ACTIVE) resetStatusLED();
    for (int i = 0; i < 2; i++){
      ESCs[i]->manualData11[0] = CMD_LED0_OFF;
      ESCs[i]->manualData11[1] = CMD_LED1_OFF;
      ESCs[i]->manualData11[2] = CMD_LED2_OFF;
      ESCs[i]->manualData11[3] = CMD_LED3_OFF;
      ESCs[i]->manualDataAmount = 4;
    }
  }
  raceMode = en;
}