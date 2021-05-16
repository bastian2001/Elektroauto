#include "global.h"
#include "messageHandler.h"
#include "wifiStuff.h"
#include "system.h"

extern WebSocketsServer webSocket;
extern bool raceMode;
extern uint8_t clients[MAX_WS_CONNECTIONS][2];
extern bool armed;
extern int ctrlMode, reqValue;
extern uint16_t cutoffVoltage, voltageWarning;

uint8_t telemetryClientsCounter = 0;
extern double pidMulti, erpmA, erpmB, erpmC;

void dealWithMessage(String message, uint8_t from) {
  #ifdef PRINT_INCOMING_MESSAGES
    Serial.print(F("Received: \""));
    Serial.print(message);
    Serial.println(F("\""));
  #endif
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
    if (valueStr == "YES" || valueStr == "TRUE") value = 1;
    setArmed(value > 0, true);
  }
  else if (command == "PING") {
    webSocket.sendTXT(from, "PONG");
  }
  else if (command == "MODE" && dividerPos != -1){
    String valueStr = message.substring(dividerPos + 1);
    valueStr.toUpperCase();
    int value = valueStr.toInt();
    if (valueStr == "RPS"){
        value = 1;
    } else if (valueStr == "SLIP"){
        value = 2;
    }
    ctrlMode = value;
    String modeText = "SET MODESPINNER ";
    modeText += ctrlMode;
    broadcastWSMessage(modeText);
    setNewTargetValue();
  }
  else if (command == "TELEMETRY" && dividerPos != -1 && from != 255){
    String valueStr = message.substring(dividerPos + 1);
    valueStr.toUpperCase();
    int value = valueStr.toInt();
    if (valueStr == "ON") value = 1;
    if (clients[from][1] > 0 && value == 0) telemetryClientsCounter--;
    if (clients[from][1] == 0 && value > 0) telemetryClientsCounter++;
    clients[from][1] = value;
    #ifdef PRINT_MEANINGS
      Serial.print("Setting telemetry of spot ");
      Serial.print(from);
      Serial.println(value > 0 ? " ON" : " OFF");
    #endif
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
    if (raceModeOn){
      broadcastWSMessage("SET RACEMODETOGGLE ON");
      broadcastWSMessage("UNBLOCK VALUE");
    } else {
      broadcastWSMessage("SET RACEMODETOGGLE OFF");
      if (!armed)
        broadcastWSMessage("BLOCK VALUE 0");
    }
    raceMode = raceModeOn;
  }
  else if (command == "STARTRACE"){
    startRace();
  }
  else if (command == "CUTOFFVOLTAGE"){
    cutoffVoltage = message.substring(dividerPos + 1).toInt();
    broadcastWSMessage(String("MESSAGE Cutoff voltage is now ") + String(cutoffVoltage), true);
  }
  else if (command == "VOLTAGEWARNING"){
    voltageWarning = message.substring(dividerPos + 1).toInt();
    broadcastWSMessage(String("MESSAGE Cutoff voltage is now ") + String(voltageWarning), true);
  }
  else if (command == "ERRORCOUNT"){
    broadcastWSMessage("MESSAGE Error-Count beträgt " + String(errorCount), true, 0, true);
    Serial.print("Error count: ");
    Serial.println(errorCount);
  }
  else if (command == "RPSA"){
    erpmA = message.substring(dividerPos + 1).toFloat();
    broadcastWSMessage(String("MESSAGE rpsA is now ") + String(erpmA), true);
  }
  else if (command == "RPSB"){
    erpmB = message.substring(dividerPos + 1).toFloat();
    broadcastWSMessage(String("MESSAGE rpsB is now ") + String(erpmB), true);
  }
  else if (command == "RPSC"){
    erpmC = message.substring(dividerPos + 1).toFloat();
    broadcastWSMessage(String("MESSAGE rpsC is now ") + String(erpmC), true);
  }
  else if (command == "RECONNECT"){
    reconnect();
  }
  else if (command == "PIDMULTIPLIER"){
    pidMulti = message.substring(dividerPos + 1).toFloat();
    broadcastWSMessage(String("MESSAGE Multiplier is now ") + String(pidMulti), true);
  }
  else if (command == "RAWDATA"){
    message = message.substring(dividerPos + 1);
    uint8_t length = message.length();
    if (length % 5 != 4 || length > 99){
      broadcastWSMessage("Invalide Länge der Rohdaten");
    } else {
      for (int i = 0; i < length; i+=5){
        uint16_t data = 0;
        for (int n = 0; n < 4; n++){
          char c = message.charAt(i + n);
          if (c >= 'A' && c <= 'F'){
            data |= ((c - 55) << ((3-n) * 4));
          } else if (c >= 'a' && c <= 'f'){
            data |= ((c - 87) << ((3-n) * 4));
          } else if (c >= '0' && c <= 'g'){
            data |= ((c - '0') << ((3-n) * 4));
          }
        }
        manualData[i/5] = data;
      }
    }
  }
}