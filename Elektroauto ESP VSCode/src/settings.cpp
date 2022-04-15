#include "global.h"
#include "WiFiHandler.h"

bool firstStartup(){
  return !(EEPROM.readBool(0));
}

void clearEEPROM(){
  EEPROM.writeBool(0, false);
  commitFlag = true;
}

void writeEEPROM(){
  EEPROM.writeBool(0, true);
  EEPROM.writeDouble(1, pidMulti);
  EEPROM.writeDouble(9, erpmA);
  EEPROM.writeDouble(17, erpmB);
  EEPROM.writeDouble(25, erpmC);
  EEPROM.writeUShort(33, ESC::getMaxThrottle());
  EEPROM.writeUShort(35, maxTargetRPS);
  EEPROM.writeUChar(37, maxTargetSlip);
  EEPROM.writeUChar(38, motorPoleCount);
  EEPROM.writeUChar(39, wheelDiameter);
  EEPROM.writeUShort(40, ESC::cutoffVoltage);
  EEPROM.writeUShort(42, warningVoltage);
  EEPROM.writeDouble(44, slipMulti);
  EEPROM.writeDouble(52, kP);
  EEPROM.writeDouble(60, kI);
  EEPROM.writeDouble(68, kD);
  commitFlag = true;
}

void readEEPROM(){
  pidMulti = EEPROM.readDouble(1);
  erpmA = EEPROM.readDouble(9);
  erpmB = EEPROM.readDouble(17);
  erpmC = EEPROM.readDouble(25);
  ESC::setMaxThrottle(EEPROM.readUShort(33));
  maxTargetRPS = EEPROM.readUShort(35);
  maxTargetSlip = EEPROM.readUChar(37);
  motorPoleCount = EEPROM.readUChar(38);
  wheelDiameter = EEPROM.readUChar(39);
  ESC::cutoffVoltage = EEPROM.readUShort(40);
  warningVoltage = EEPROM.readUShort(42);
  slipMulti = EEPROM.readDouble(44);
  kP = EEPROM.readDouble(52);
  kI = EEPROM.readDouble(60);
  kD = EEPROM.readDouble(68);
}

void sendSettings(uint8_t to){
    char settingsString[620];
    snprintf(settingsString, 620, "SETTINGS\nfloat_PGAIN_P gain_0_1_%3.2f\nfloat_IGAIN_I gain_0_0.1_%4.3f\nfloat_DGAIN_D gain_0_5_%5.4f\nint_MAXT_Max. Gaswert_0_2000_%d\nint_MAXR_Max. U/sek_0_2000_%d\nint_MAXS_Max. Schlupf_0_30_%d\nint_MOTORPOLE_Anzahl Motorpole_0_20_%d\nint_WHEELDIA_Raddurchmesser (mm)_0_50_%d\nint_CUTOFFVOLTAGE_Cutoffspannung (cV)_0_2000_%d\nint_WARNINGVOLTAGE_Warnungsspannung (cV)_0_2000_%d", kP, kI, kD, ESC::getMaxThrottle(), maxTargetRPS, maxTargetSlip, motorPoleCount, wheelDiameter, ESC::cutoffVoltage, warningVoltage);
    // snprintf(settingsString, 620, "SETTINGS\nfloat_PIDMULTIPLIER_RPS Multiplikator_0_3_%3.2f\nfloat_SLIPMULTIPLIER_Zusätzlicher Multiplikator für Schlupf_1_10_%4.2f\nfloat_RPSA_Dritte Potenz RPS_0_0.00000002_%11.10f\nfloat_RPSB_Zweite Potenz RPS_0_0.000001_%10.9f\nfloat_RPSC_Linearfaktor RPS_0_0.02_%5.4f\nint_MAXT_Max. Gaswert_0_2000_%d\nint_MAXR_Max. U/sek_0_2000_%d\nint_MAXS_Max. Schlupf_0_30_%d\nint_MOTORPOLE_Anzahl Motorpole_0_20_%d\nint_WHEELDIA_Raddurchmesser (mm)_0_50_%d\nint_CUTOFFVOLTAGE_Cutoffspannung (cV)_0_2000_%d\nint_WARNINGVOLTAGE_Warnungsspannung (cV)_0_2000_%d", pidMulti, slipMulti, erpmA, erpmB, erpmC, ESC::getMaxThrottle(), maxTargetRPS, maxTargetSlip, motorPoleCount, wheelDiameter, ESC::cutoffVoltage, warningVoltage);
    sendWSMessage(to, String(settingsString));
}