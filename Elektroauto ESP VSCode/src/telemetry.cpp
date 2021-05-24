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
bool firstTelemetry = true;

uint8_t get_crc8(uint8_t *Buf, uint8_t BufLen);

/**
 * @brief acquires telemetry data
 * 
 * reads telemetry data from Serial2 buffer
 * converts the values to a readable format
 * checks for overheating/cutoffvoltage/too high rpm
 * sets LEDs to green only after bootup
 */
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
      if(firstTelemetry){
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

/**
 * @brief sends the telemtry to all connected phones
 * 
 * telemtry format is documented in docs.md
 */
void sendTelemetry() {
  float rps = erpmToRps (telemetryERPM);
  int slipPercent = 0;
  if (speedWheel != 0) {
    slipPercent = (float)(speedWheel - speedMPU) / speedWheel * 100;
  }
  char telData[66];
  snprintf(telData, 66, "TELEMETRY a%d!p%d!u%d!t%d!r%d!s%d!v%d!w%d!c%d!%c%d", 
    armed > 0, telemetryTemp, telemetryVoltage, (int) throttle, (int) rps, slipPercent, (int) speedMPU, speedWheel,
    (int)(acceleration * 1000 + .5), (raceMode && !raceActive) ? 'o' : 'q', reqValue);
  broadcastWSMessage(telData, true, 0, true);
}

/**
 * @brief checks if telemetry is ready to be processed
 * 
 * @return whether telemetry can be read
 */
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

//! @brief updates the crc checksum
uint8_t update_crc8(uint8_t crc, uint8_t crc_seed){
  uint8_t crc_u, i;
  crc_u = crc;
  crc_u ^= crc_seed;
  for ( i=0; i<8; i++)
    crc_u = ( crc_u & 0x80 ) ? 0x7 ^ ( crc_u << 1 ) : ( crc_u << 1 );
  return (crc_u);
}

/**
 * @brief Get the crc checksum of given data
 * 
 * @param Buf pointer to a memory area to create the checksum of
 * @param BufLen the buffer length
 * @return the checksum 
 */
uint8_t get_crc8(uint8_t *Buf, uint8_t BufLen){
  uint8_t crc = 0, i;
  for( i=0; i<BufLen; i++)
    crc = update_crc8(Buf[i], crc);
  return (crc);
}