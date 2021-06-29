#include <Arduino.h>
#include "global.h"
#include "telemetry.h"
#include "wifiStuff.h"
#include "system.h"

char escTelemetry[10];
uint32_t lastErrorOutput = 0;
bool firstTelemetry = true;

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
      for (uint8_t i = 0; i < 10; i++){
        escTelemetry[i] = 1;
      }
      if (telemetryTemp > 80){
        if (armed){
          setArmed(false);
          broadcastWSMessage("MESSAGEBEEP Wegen Überhitzung disarmed!");
          manualData[0] = 0xbb;
          manualDataAmount = 1;
        }
      } else if(telemetryVoltage < cutoffVoltage){
        if (armed){
          setArmed(false);
          broadcastWSMessage("MESSAGEBEEP Cutoff-Spannung unterschritten!");
          manualData[19] = 0xbb;
          manualDataAmount = 20; // delay so the voltage can (maybe) rise up a little
        }
      } else if (telemetryERPM > 8000){
        errorCount++;
        if (armed){
          setArmed(false);
          broadcastWSMessage("MESSAGEBEEP Zu hohe Drehzahl");
          manualData[19] = 0xbb;
          manualDataAmount = 20; // delay for (maybe) a little slowdown
        }
        if (lastErrorOutput + 100 < millis()){
          char errorMessage[60];
          snprintf(errorMessage, 60, "Error: ESC-Temperatur: %d°C, Spannung: %4.2fV, ERPM: %d",
            telemetryTemp, (double)telemetryVoltage / 100.0, telemetryERPM);
          lastErrorOutput = millis();
          sPrintln(errorMessage);
        }
      }
      if(firstTelemetry){ //setup after connection is ready
        firstTelemetry = false;
        manualData[17] = 0x0356;
        manualData[18] = 0x02FD;
        manualData[19] = 0x039A;
        manualDataAmount = 20;
      }
      break;
    }
  }
}

bool isTelemetryComplete(){
  if (escTelemetry[0] > 1
    && get_crc8((uint8_t*)escTelemetry, 9) == escTelemetry[9]
    && escTelemetry[3] == 0
    && escTelemetry[4] == 0
    && escTelemetry[5] == 0
    && escTelemetry[6] == 0
    && escTelemetry[0] < 100
    && escTelemetry[1] < 8
    )
    return true;
  return false;
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