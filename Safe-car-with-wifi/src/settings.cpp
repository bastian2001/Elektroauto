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
  EEPROM.writeUShort(33, ESC::getMaxThrottle());
  commitFlag = true;
}

void readEEPROM(){
  ESC::setMaxThrottle(EEPROM.readUShort(33));
  escPassthroughMode = EEPROM.readUChar(76);
}

void sendSettings(uint8_t to){
    char settingsString[1000];
    snprintf(settingsString, 1000, "SETTINGS\nint_MAXT_Max. Gaswert_0_2000_%d", ESC::getMaxThrottle());
    sendWSMessage(to, String(settingsString));
}