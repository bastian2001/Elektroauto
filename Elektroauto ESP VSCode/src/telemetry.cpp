#include <Arduino.h>
#include "global.h"
#include "telemetry.h"
#include "wifiStuff.h"
#include "system.h"

uint16_t errorCount = 0;
extern bool raceMode, raceActive;
extern int previousERPM[TREND_AMOUNT];
uint16_t telemetryERPM = 0, telemetryVoltage = 0;
uint8_t telemetryTemp = 0;
extern bool armed;
extern int reqValue;
extern double throttle;
char escTelemetry[10];
uint16_t speedWheel = 0;
extern bool updatedValue;

void getTelemetry(){
  while(Serial2.available()){
    for (uint8_t i = 0; i < 9; i++){
      escTelemetry[i] = escTelemetry[i+1];
    }
    escTelemetry[9] = (char) Serial2.read();
    if (isTelemetryComplete()){
      updatedValue = true;
      telemetryTemp = escTelemetry[0];
      telemetryVoltage = (escTelemetry[1] << 8) | escTelemetry[2];
      telemetryERPM = (escTelemetry[7] << 8) | escTelemetry[8];
      speedWheel = (float)telemetryERPM * ERPM_TO_MM_PER_SECOND / GEAR_RATIO;
      for (uint8_t i = 0; i < 9; i++){
        escTelemetry[i] = B1;
      }
      if (telemetryVoltage == 257){
        setArmed(false);
      }
      if (telemetryTemp > 140 || telemetryVoltage > 2000 || telemetryVoltage < 500 || telemetryERPM > 8000){
        errorCount++;
        // Serial.println("error");
        break;
      }
      break;
    }
  }
}

void sendTelemetry() {
  float rps = erpmToRps (telemetryERPM);
  int slipPercent = 0;
  if (speedWheel != 0) {
    slipPercent = (float)(speedWheel - speedMPU) / speedWheel * 100;
  }
  //String telemetryData2 = "";
  String telemetryData = armed ? "TELEMETRY a1!p" : "TELEMETRY a0!p";
  telemetryData += telemetryTemp;
  telemetryData += "!u";
  telemetryData += telemetryVoltage;
  telemetryData += "!t";
  telemetryData += (int) throttle;
  telemetryData += "!r";
  telemetryData += (int) rps;
  telemetryData += "!s";
  telemetryData += slipPercent;
  telemetryData += "!v";
  telemetryData += (int) speedMPU;
  telemetryData += "!w";
  telemetryData += speedWheel;
  telemetryData += "!c";
  telemetryData += ((int)(acceleration * 1000 + .5));
  telemetryData += (raceMode && !raceActive) ? "!o" : "!q";
  telemetryData += reqValue;
  broadcastWSMessage(telemetryData, true, 0, true);
}

bool isTelemetryComplete(){
  return escTelemetry[3] == 0 && escTelemetry[4] == 0 && escTelemetry[5] == 0 && escTelemetry[6] == 0;
}