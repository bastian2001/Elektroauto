#include "global.h"
#include "wifiStuff.h"
#include "system.h"
#include "messageHandler.h"

double pidMulti = 1, erpmA = 0.0000000008, erpmB = 0.00000006, erpmC = 0.001;
int escOutputCounter2 = 0;
bool raceMode = false;
extern bool armed;
int targetERPM = 0, ctrlMode = 0, reqValue = 0, targetSlip = 0;
uint16_t escValue = 0x0011;
extern double throttle;
bool redLED, greenLED, blueLED;

//! voltage warning
uint16_t cutoffVoltage = 600, voltageWarning = 720;
uint32_t nextCheck = 0;
uint8_t voltageWarningCount = 0;

//! race mode
uint16_t throttle_log[LOG_FRAMES], erpm_log[LOG_FRAMES], voltage_log[LOG_FRAMES];
int acceleration_log[LOG_FRAMES];
uint8_t temp_log[LOG_FRAMES];
bool raceModeSendValues = false;

/**
 * @brief arms or disarms the car
 * 
 * always use this function as it also sets the throttle to a safe 0 if the race is inactive
 * @param arm whether to arm or disarm the car
 */
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

/**
 * @brief Sets the ESC value with the appended checksum
 * 
 * WARNING: There are no further armed checks between this function and the sending of the value, alter the "nextThrottle" variable instead. The offset for the settings is automatically applied
 * @param newThrottle the new throttle between 0 and 2000, value is floored
 */
void setThrottle(double newThrottle) {
  newThrottle = (newThrottle > 2000) ? 2000 : newThrottle;
  newThrottle = (newThrottle < 0) ? 0 : newThrottle;
  newThrottle = (newThrottle > MAX_THROTTLE) ? MAX_THROTTLE : newThrottle;
  throttle = newThrottle;
  newThrottle += (newThrottle == 0) ? 0 : 47;
  escValue = appendChecksum(newThrottle);
}

/**
 * @brief appends the checksum and the telemtry request bit
 * 
 * @param value the first 11 bits from 0 to 2047
 * @param telemetryRequest default: true, whether or not to set the telemetry request bit
 * @return the full escValue packet
 */
uint16_t appendChecksum(uint16_t value, bool telemetryRequest) {
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

//! @brief starts the race
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

/**
 * @brief calculates the next throttle value in speed control mode
 * 
 * @param target the target hERPM value
 * @param was array of previous hERPM value
 * @param masterMultiplier default: 1, another GP multiplier for quick adjustments
 * @return the next throttle value (NOT set automatically)
 */
double calcThrottle(int target, int was[], double masterMultiplier) {
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
  delta_throttle *= pidMulti * masterMultiplier;

  return throttle + delta_throttle;
}

/// @brief reads the Serial0 port and processes the message using dealWithMessage
void receiveSerial() {
  if (Serial.available()) {
    String readout = Serial.readStringUntil('\n');
    dealWithMessage(readout, 255);
  }
}

/**
 * @brief converts RPS to hERPM
 * 
 * @param rps the speed in RPS
 * @return the speed in hERPM
 */
float rpsToErpm(float rps){
  return (rps / RPS_CONVERSION_FACTOR + .5f);
}
/**
 * @brief converts hERPM to RPS
 * 
 * @param erpm the speed in hERPM
 * @return the speed in RPS
 */
float erpmToRps(float erpm){
  return (erpm * RPS_CONVERSION_FACTOR + .5f);
}

/**
 * @brief Set the new target value based on reqValue
 * 
 * This is not the throttle routine. It merely sets the target value based on the current mode and reqValue
 */
void setNewTargetValue(){
  switch (ctrlMode) {
    case 0:
      nextThrottle = reqValue;
      break;
    case 1:
      if (reqValue > MAX_TARGET_RPS)
        reqValue = MAX_TARGET_RPS;
      targetERPM = rpsToErpm(reqValue);
      break;
    case 2:
      if (reqValue > MAX_TARGET_SLIP)
        reqValue = MAX_TARGET_SLIP;
      targetSlip = reqValue;
      break;
    default:
      break;
  }
}

//! @brief sends the race log to all connected phones
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
}

/**
 * @brief throttle routine
 * 
 * calculates the nextThrottle if neccessary (if in mode 1 or 2), automatically executed as often as possible
 * the variable throttle is not mutated, thus the frequency this method is eqecuted is almost irrelevant
 */
void throttleRoutine(){
  if (armed) {
    switch (ctrlMode) {
      int addToTargetERPM;
      case 0:
        break;
      case 1:
        nextThrottle = calcThrottle(targetERPM, previousERPM);
        break;
      case 2:
        addToTargetERPM = 40 - (throttle * .2);
        if (addToTargetERPM < 0)
          addToTargetERPM = 0;
        targetERPM = ((0.0f - speedMPU) / ((float) targetSlip * .01f - 1)) / ERPM_TO_MM_PER_SECOND + addToTargetERPM;
        nextThrottle = calcThrottle(targetERPM, previousERPM, .1);
        break;
      default:
        break;
    }
  } 
}

//! @brief checks every 10s if the voltage is two low, initiates warning via broadcast if neccessary
void checkVoltage(){
  if (millis() > nextCheck){
    nextCheck += 10000;
    if (telemetryVoltage < voltageWarning && telemetryVoltage != 0 && telemetryVoltage != 257){
      voltageWarningCount++;
      if (voltageWarningCount % 5 == 3){
        broadcastWSMessage(F("MESSAGEBEEP Warnung! Spannung niedrig!"));
      }
    } else {
      voltageWarningCount = 0;
    }
  }
}