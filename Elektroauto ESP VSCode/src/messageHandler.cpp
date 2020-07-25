#include "global.h"
#include "messageHandler.h"
#include "wifiStuff.h"
#include "system.h"

extern WebSocketsServer webSocket;
extern bool raceMode;
extern uint8_t clients[MAX_WS_CONNECTIONS][2];
extern bool armed, newValueFlag;
extern int ctrlMode, reqValue;

uint8_t telemetryClientsCounter;
extern double pidMulti, rpsA, rpsB, rpsC;
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
    newValueFlag = true;
  } else if (command == "ARMED" && dividerPos != -1){
    String valueStr = message.substring(dividerPos + 1);
    valueStr.toUpperCase();
    int value = valueStr.toInt();
    if (valueStr == "YES" || valueStr == "TRUE") value = 1;
    setArmed(value > 0, true);
  } else if (command == "PING") {
    webSocket.sendTXT(from, "PONG");
  } else if (command == "MODE" && dividerPos != -1){
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
    newValueFlag = true;
  } else if (command == "TELEMETRY" && dividerPos != -1 && from != 255){
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
  } else if (command == "DEVICE" && dividerPos != -1 && from != 255){
    String valueStr = message.substring(dividerPos + 1);
    int value = valueStr.toInt();
    if (valueStr == "APP") value = 1;
    clients[from][0] = value;
  } else if (command == "RACEMODE" && dividerPos != -1){
    String valueStr = message.substring(dividerPos + 1);
    valueStr.toUpperCase();
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
  } else if (command == "STARTRACE"){
    startRace();
  // } else if (command == "CUTOFFVOLTAGE"){
    // cutoffVoltage = message.substring(dividerPos + 1).toInt();
  } else if (command == "RPSA"){
    rpsA = message.substring(dividerPos + 1).toFloat();
  } else if (command == "RPSB"){
    rpsB = message.substring(dividerPos + 1).toFloat();
  } else if (command == "RPSC"){
    rpsC = message.substring(dividerPos + 1).toFloat();
  } else if (command == "RECONNECT"){
    reconnect();
  } else if (command == "PIDMULTIPLIER"){
    pidMulti = message.substring(dividerPos + 1).toFloat();
  }
}