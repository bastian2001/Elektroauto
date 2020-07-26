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

void getTelemetry(){
  while(Serial2.available()){
    for (uint8_t i = 0; i < 9; i++){
      escTelemetry[i] = escTelemetry[i+1];
    }
    escTelemetry[9] = (char) Serial2.read();
    if (isTelemetryComplete()){
      telemetryTemp = escTelemetry[0];
      telemetryVoltage = (escTelemetry[1] << 8) | escTelemetry[2];
      telemetryERPM = (escTelemetry[7] << 8) | escTelemetry[8];
      for (uint8_t i = 0; i < 9; i++){
        escTelemetry[i] = B1;
      }
      if (telemetryVoltage == 257/*telemetryVoltage < cutoffVoltag*/){
        // bool pArmed = armed;
        setArmed(false);
        // if (pArmed)
        //   broadcastWSMessage("MESSAGE Cutoff-Spannung unterschritten. Gerät disarmed", true);
      }
      if (telemetryTemp > 140 || telemetryVoltage > 2000 || telemetryVoltage < 500 || telemetryERPM > 2000){
        errorCount++;
        // Serial.println("error");
        break;
      }
      break;
    }
  }
}

void sendTelemetry() {
  int velMPU = (int)(speedMPU * 1000 + .5);
  int velWheel = (int)((float)30 * PI * (float)previousERPM[TREND_AMOUNT - 1]);
  int slipPercent = 0;
  if (velWheel != 0) {
    slipPercent = (float)(velWheel - velMPU) / velWheel * 100;
  }
  //String telemetryData2 = "";
  String telemetryData = armed ? "TELEMETRY a1!p" : "TELEMETRY a0!p";
  telemetryData += telemetryTemp;
  telemetryData += "!u";
  telemetryData += telemetryVoltage;
  telemetryData += "!t";
  telemetryData += (int) throttle;
  telemetryData += "!r";
  telemetryData += erpmToRps(telemetryERPM);
  telemetryData += "!s";
  telemetryData += slipPercent;
  telemetryData += "!v";
  telemetryData += velMPU;
  telemetryData += "!w";
  telemetryData += velWheel;
  telemetryData += "!c";
  telemetryData += ((int)(acceleration * 1000 + .5));
  telemetryData += (raceMode && !raceActive) ? "!o" : "!q";
  telemetryData += reqValue;
  broadcastWSMessage(telemetryData, true, 0, true);
}

bool isTelemetryComplete(){
  return escTelemetry[3] == 0 && escTelemetry[4] == 0 && escTelemetry[5] == 0 && escTelemetry[6] == 0;
}