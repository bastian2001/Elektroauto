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
extern uint16_t cutoffVoltage;
uint32_t lastErrorOutput = 0;

uint8_t get_crc8(uint8_t *Buf, uint8_t BufLen);

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
      speedWheel = (float)telemetryERPM * ERPM_TO_MM_PER_SECOND;
      for (uint8_t i = 0; i < 9; i++){
        escTelemetry[i] = 1;
      }
      if (telemetryVoltage == 257){
        // setArmed(false);
      } else if (telemetryTemp > 80 || telemetryVoltage < cutoffVoltage || telemetryERPM > 8000){
        errorCount++;
        // setArmed(false);
        if (lastErrorOutput + 100 < millis()){
          String eMessage = "Error: ESC-Temperatur: " + String(telemetryTemp);
          eMessage += ", Spannung: " + String(telemetryVoltage / 100);
          eMessage += "." + (((telemetryVoltage % 100 < 10) ? "0":"" ) + String(telemetryVoltage % 100));
          eMessage += "V, ERPM: " + String(telemetryERPM);
          lastErrorOutput = millis();
          sPrintln(eMessage);
        }
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
  if (escTelemetry[3] != 0 || escTelemetry[4] != 0 || escTelemetry[5] != 0 || escTelemetry[6] != 0 || escTelemetry[0] == 1)
    return false;
  uint8_t cs = get_crc8((uint8_t*)escTelemetry, 9);
  if(cs != escTelemetry[9]){
    for (int i = 0; i < 10; i++){
      Serial.print((int)escTelemetry[i]);
      Serial.print(" ");
    }
    Serial.println();
  }
  return cs == escTelemetry[9];
}

uint8_t update_crc8(uint8_t crc, uint8_t crc_seed){
  uint8_t crc_u, i;
  crc_u = crc;
  crc_u ^= crc_seed;
  for ( i=0; i<8; i++)
    crc_u = ( crc_u & 0x80 ) ? 0x7 ^ ( crc_u << 1 ) : ( crc_u << 1 );
  return (crc_u);
}

uint8_t get_crc8(uint8_t *Buf, uint8_t BufLen){
  uint8_t crc = 0, i;
  for( i=0; i<BufLen; i++)
    crc = update_crc8(Buf[i], crc);
  return (crc);
}