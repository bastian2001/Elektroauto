#include "global.h"
#include "messageHandler.h"
#include "WiFiHandler.h"
#include "settings.h"
#include "system.h"

void processMessage(String message, uint8_t from) {
  #ifdef PRINT_INCOMING_MESSAGES
    Serial.print(F("Received: \""));
    Serial.print(message);
    Serial.println(F("\""));
  #endif

  // divides the message into command and payload
  int dividerPos = message.indexOf(":");
  String command = dividerPos == -1 ? message : message.substring(0, dividerPos);
  
  if (command == "VALUE" && dividerPos != -1){
    reqValue = message.substring(dividerPos + 1).toInt();
    setNewTargetValue();
  }

  else if (command == "ARMED" && dividerPos != -1){
    String valueStr = message.substring(dividerPos + 1);
    valueStr.toUpperCase();
    int value = valueStr.toInt();
    if (value == 1 || valueStr == "YES" || valueStr == "TRUE"){
      setArmed(true);
      return;
    }
    setArmed(false);
  }

  else if (command == "PING") {
    sendWSMessage(from, "PONG");
  }

  else if (command == "MANUALRACE"){
    automaticRace = false;
  }

  else if (command == "AUTORACE" || command == "AUTOMATICRACE"){
    automaticRace = true;
  }

  else if (command == "TELEMETRY" && dividerPos != -1 && from != 255){
    String valueStr = message.substring(dividerPos + 1);
    valueStr.toUpperCase();
    int value = valueStr.toInt();
    if (valueStr == "ON") value = 1;
    if (clients[from][1] > 0 && value == 0) telemetryClientsCounter--;
    if (clients[from][1] == 0 && value > 0) telemetryClientsCounter++;
    clients[from][1] = value;
  }

  else if (command == "DEVICE" && dividerPos != -1 && from != 255){
    String valueStr = message.substring(dividerPos + 1);
    int value = valueStr.toInt();
    if (valueStr == "APP") value = 1;
    clients[from][0] = value;
  }

  else if (command == "RACEMODE" && dividerPos != -1){
    String valueStr = message.substring(dividerPos + 1);
    bool raceModeOn = valueStr.toInt() > 0;
    if (valueStr == "ON") raceModeOn = 1;
    enableRaceMode(raceModeOn);
  }

  else if (command == "STARTRACE"){
    startRace();
  }

  else if (command == "RECONNECT"){
    reconnect();
  }

  else if (command == "RAWDATA"){
    message = message.substring(dividerPos + 1);
    uint8_t length = message.length();
    if (length % 4 != 3 || length > MAX_MANUAL_DATA * 4 - 1){
      sendWSMessage(from, F("MESSAGE Invalide Länge der Rohdaten!"));
    } else {
      for (int i = 0; i < length; i+=4){
        uint16_t data = 0;
        for (int n = 0; n < 3; n++){
          char c = message.charAt(i + n);
          if (c >= 'A' && c <= 'F'){
            data |= ((c - 55) << ((2-n) * 4));
          } else if (c >= 'a' && c <= 'f'){
            data |= ((c - 87) << ((2-n) * 4));
          } else if (c >= '0' && c <= 'g'){
            data |= ((c - '0') << ((2-n) * 4));
          }
        }
        ESCs[0]->manualData11[i/4] = data;
        ESCs[1]->manualData11[i/4] = data;
      }
      ESCs[0]->manualDataAmount = length / 4 + 1;
      ESCs[1]->manualDataAmount = length / 4 + 1;
    }
  }

  else if (command == "SAVESETTINGS" || command == "SAVE"){
    writeEEPROM();
    sendWSMessage(from, "MESSAGE Einstellungen gespeichert");
  }
  else if (command == "READSETTINGS" || command == "READ"){
    readEEPROM();
    sendWSMessage(from, "MESSAGE Einstellungen gelesen");
  }
  else if (command == "RESTORE" || command == "RESTOREDEFAULTS"){
    clearEEPROM();
    sendWSMessage(from, "MESSAGE Einstellungen zurückgesetzt. Neustart wird durchgeführt.");
    delay(200);
    ESP.restart();
  }
  else if (command == "SENDSETTINGS" || command == "SETTINGS") {
    sendSettings(from);
  }
  else if (command == "MAXT" || command == "MAXTHROTTLE"){
    ESC::setMaxThrottle(message.substring(dividerPos + 1).toInt());
    broadcastWSMessage(String("MAXVALUE ") + String(ESC::getMaxThrottle()));
    if (!message.endsWith("NOMESSAGE"))
      sendWSMessage(from, String("MESSAGE Max. Gaswert ist nun ") + String(ESC::getMaxThrottle()));
  }

  else if (command == "DIRECTION"){
    String valueStr = message.substring(dividerPos + 1);
    uint8_t dir = valueStr.charAt(0) == 'F';
    if (valueStr.charAt(0) != 'B' || dir) dir = 2;
    if (dir == 2){ sendWSMessage(from, "MESSAGE Invalide Richtung"); return;}
    uint8_t esc = valueStr.length() > 1 ? (valueStr.charAt(valueStr.length() - 1) == 'L' ? 0 : 1) : 2;
    uint16_t cmd = dir ? CMD_SPIN_DIRECTION_NORMAL : CMD_SPIN_DIRECTION_REVERSED;
    if (esc == 0 || esc == 2){
      for (int i = 0; i < 12; i++){
        ESCs[0]->manualData11[i + ESCs[0]->manualDataAmount] = cmd;
      }
      ESCs[0]->manualDataAmount = ESCs[0]->manualDataAmount + 12;
    }
    if (esc == 1 || esc == 2){
      for (int i = 0; i < 12; i++){
        ESCs[1]->manualData11[i + ESCs[1]->manualDataAmount] = cmd;
      }
      ESCs[1]->manualDataAmount = ESCs[1]->manualDataAmount + 12;
    }
  }

  else if (command == "ESCCONFIG" || command == "ESCPASSTHROUGH" || command == "ESCCONF" || command == "PASSTHROUGH" || command == "BLHELI"){
    char which = message.charAt(dividerPos + 1);
    switch (which){
      case 'L':
      case '0':
      case 'l':
        escPassthroughMode = ESC1_OUTPUT_PIN;
        break;
      case 'R':
      case 'r':
      case '1':
        escPassthroughMode = ESC2_OUTPUT_PIN;
        break;
      default:
        escPassthroughMode = message.substring(dividerPos + 1).toInt();
    }
    if (escPassthroughMode){
      sendWSMessage(from, String("MESSAGE ESC Passthrough wird auf Pin ") + String(escPassthroughMode) + String(" gestartet"));
      EEPROM.writeUChar(76, escPassthroughMode);
      commitFlag = true;
    } else {
      sendWSMessage(from, "MESSAGE Unbekannter Pin/ESC");
    }
  }
  
  else if (command == "REBOOT" || command == "RESTART"){
    ESP.restart();
  }
}