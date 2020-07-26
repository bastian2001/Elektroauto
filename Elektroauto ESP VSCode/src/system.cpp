#include "global.h"
#include "wifiStuff.h"
#include "system.h"
#include "messageHandler.h"
#include "ArduinoJson.h"

double pidMulti = 1.5, erpmA = 0.00000008, erpmB = 0.000006, erpmC = 0.01;
int escOutputCounter2 = 0;
bool raceMode = false;
extern bool armed;
extern int reqValue, targetERPM;
uint16_t escValue = 0;
extern double throttle;

//raceMode
uint16_t throttle_log[LOG_FRAMES], erpm_log[LOG_FRAMES], voltage_log[LOG_FRAMES];
int acceleration_log[LOG_FRAMES];
uint8_t temp_log[LOG_FRAMES];
bool raceModeSendValues = false;

void setArmed (bool arm, bool sendNoChangeBroadcast){
  if (arm != armed){
    if (!raceActive){
      reqValue = 0;
      targetERPM = 0;
    }
    broadcastWSMessage((raceActive ? arm : arm || raceMode) ? "UNBLOCK VALUE" : "BLOCK VALUE 0");
    armed = arm;
    setThrottle(0);
  } else if (sendNoChangeBroadcast){
    broadcastWSMessage(arm ? "MESSAGE already armed" : "MESSAGE already disarmed");
  }
}

void setThrottle(double newThrottle) { //throttle value between 0 and 2000 --> esc value between 0 and 2047 with checksum
  newThrottle = (newThrottle > 2000) ? 2000 : newThrottle;
  newThrottle = (newThrottle < 0) ? 0 : newThrottle;
  newThrottle = (newThrottle > MAX_THROTTLE) ? MAX_THROTTLE : newThrottle;
  throttle = newThrottle;
  newThrottle += (newThrottle == 0) ? 0 : 47;
  escValue = appendChecksum(newThrottle);
}

uint16_t appendChecksum(uint16_t value) {
  value &= 0x7FF;
  escOutputCounter2 = (escOutputCounter2 == ESC_TELEMETRY_REQUEST) ? 0 : escOutputCounter2 + 1;
  bool telem = escOutputCounter2 == 0;
  value = (value << 1) | telem;
  int csum = 0, csum_data = value;
  for (int i = 0; i < 3; i++) {
    csum ^=  csum_data;   // xor data by nibbles
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
    setNewValue();
  } else if (!raceMode) {
    broadcastWSMessage("SET RACEMODETOGGLE OFF");
  }
}

double calcThrottle(int target, int was[]) {
  double was_avg = 0;
  int was_sum = 0, t_sq_sum = 0, t_multi_was_sum = 0;
  for (int i = 0; i < TREND_AMOUNT; i++) {
    int t = i - TA_DIV_2;
    was_sum += was[i];
    t_sq_sum += pow(t, 2);
    t_multi_was_sum += t * was[i];
  }
  was_avg = (double)was_sum / TREND_AMOUNT;

  double m = (double)t_multi_was_sum / (double)t_sq_sum;

  double prediction = m * ((double)TREND_AMOUNT - (double)TA_DIV_2) + (double)was_avg;
  if (prediction < 0) prediction = 0;
  double deltaERPM = target - prediction;
  double delta_throttle = erpmA * pow(deltaERPM, 3) + erpmB * pow(deltaERPM, 2) + erpmC * deltaERPM;
  delta_throttle *= pidMulti;

  throttle += delta_throttle;

  return throttle;
}

void receiveSerial() {
  if (Serial.available()) {
    String readout = Serial.readStringUntil('\n');
    dealWithMessage(readout, 255);
  }
}

int rpsToErpm(float rps){
  return (rps / RPS_CONVERSION_FACTOR + .5f);
}
int erpmToRps(float erpm){
  return (erpm * RPS_CONVERSION_FACTOR + .5f);
}

void setNewValue(){
  switch (ctrlMode) {
    case 0:
      setThrottle(reqValue);
      break;
    case 1:
      targetERPM = rpsToErpm(reqValue);
      break;
    case 2:
      break;
    default:
      break;
  }
}

void sendRaceLog(){
  raceModeSendValues = false;
  uint8_t logData[LOG_FRAMES * 9];
  memcpy(logData, throttle_log, LOG_FRAMES * 2);
  memcpy(logData + LOG_FRAMES * 2, acceleration_log, LOG_FRAMES * 2);
  memcpy(logData + LOG_FRAMES * 4, erpm_log, LOG_FRAMES * 2);
  memcpy(logData + LOG_FRAMES * 6, voltage_log, LOG_FRAMES * 2);
  memcpy(logData + LOG_FRAMES * 8, temp_log, LOG_FRAMES);
  delay(2);
  broadcastWSBin(logData, LOG_FRAMES * 9, true, 20);
  Serial2.begin(115200);
}