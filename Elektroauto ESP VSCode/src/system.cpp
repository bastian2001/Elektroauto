#include "global.h"
#include "wifiStuff.h"
#include "system.h"
#include "messageHandler.h"

extern double pidMulti, rpsA, rpsB, rpsC;
extern int escOutputCounter2;
extern bool raceMode, raceActive;
extern bool armed, newValueFlag;
extern int reqValue, targetRPS;
extern uint16_t escValue;
extern double throttle;

void setArmed (bool arm, bool sendBroadcast){
  if (arm != armed){
    if (!raceActive){
      reqValue = 0;
      targetRPS = 0;
    }
    broadcastWSMessage(arm ? "UNBLOCK VALUE" : "BLOCK VALUE 0");
    armed = arm;
    setThrottle(0);
  } else if (sendBroadcast){
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
    newValueFlag = true;
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
  double delta_rps = target - prediction;
  double delta_throttle = rpsA * pow(delta_rps, 3) + rpsB * pow(delta_rps, 2) + rpsC * delta_rps;
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