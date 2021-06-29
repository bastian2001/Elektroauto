//! @file messageHandler.cpp function for message handling

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

/*! @brief processes Serial- and WebSocket messages
 * 
 * Message more precisely documented in docs.md
 * - VALUE command for requesting a value
 * - ARMED command for arming the car
 * - PING command for replying with PONG / pinging the connection
 * - MODE command for setting the driving mode
 * - TELEMETRY command for setting telemetry of the origin on or off
 * - DEVICE command for setting the device type
 * - RACEMODE command for enabling or disabling race mode
 * - STARTRACE command for starting the race
 * - CUTOFFVOLTAGE, VOLTAGEWARNING, RPSA, RPSB, RPSC and PIDMULTIPLIER command for setting their respective parameters 
 * - ERRORCOUNT for requesting the error count
 * - RECONNECT for reconnecting to WiFi
 * - RAWDATA for sending raw data to the ESC
 * @param message The message
 * @param from The origin of the message, 255 for Serial, other values for WebSocket spots
*/
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
    if (valueStr == "YES" || valueStr == "TRUE") value = 1;
    setArmed(value > 0);
  }

  else if (command == "PING") {
    sendWSMessage(from, "PONG");
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
    switch(ctrlMode){
      case 0:
        reqValue = nextThrottle; // stationary
        break;
      case 1:
        if (erpmToRps(telemetryERPM) > MAX_TARGET_RPS)
          reqValue = MAX_TARGET_RPS;
        reqValue = erpmToRps(telemetryERPM);
        targetERPM = rpsToErpm(reqValue);
        break;
      case 2:
        if (reqValue > MAX_TARGET_SLIP)
          reqValue = MAX_TARGET_SLIP;
        targetSlip = reqValue;
        break;
    }
    broadcastWSMessage(modeText);
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
    char bcMessage[50];
    snprintf(bcMessage, 50, "MESSAGE Not-Stop erfolgt nun unter %4.2fV", (double)cutoffVoltage/100.0);
    sendWSMessage(from, bcMessage);
  }
  else if (command == "VOLTAGEWARNING"){
    voltageWarning = message.substring(dividerPos + 1).toInt();
    char bcMessage[50];
    snprintf(bcMessage, 50, "MESSAGE Spannungswarnung erfolgt unter %4.2fV", (double)voltageWarning/100);
    sendWSMessage(from, bcMessage);
  }
  else if (command == "RPSA"){
    erpmA = message.substring(dividerPos + 1).toFloat();
    sendWSMessage(from, String("MESSAGE Tempomat 3. Potenz ist nun ") + String(erpmA));
  }
  else if (command == "RPSB"){
    erpmB = message.substring(dividerPos + 1).toFloat();
    sendWSMessage(from, String("MESSAGE Tempomat 2. Potenz ist nun ") + String(erpmB));
  }
  else if (command == "RPSC"){
    erpmC = message.substring(dividerPos + 1).toFloat();
    sendWSMessage(from, String("MESSAGE Tempomat 1. Potenz ist nun ") + String(erpmC));
  }
  else if (command == "PIDMULTIPLIER"){
    pidMulti = message.substring(dividerPos + 1).toFloat();
    sendWSMessage(from, String("MESSAGE Tempomat-Master ist nun ") + String(pidMulti));
  }

  else if (command == "ERRORCOUNT"){
    sendWSMessage(from, "MESSAGE Error-Count beträgt " + String(errorCount));
  }

  else if (command == "RECONNECT"){
    reconnect();
  }

  else if (command == "RAWDATA"){
    message = message.substring(dividerPos + 1);
    uint8_t length = message.length();
    if (length % 5 != 4 || length > 99){
      sendWSMessage(from, F("MESSAGE Invalide Länge der Rohdaten!"));
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
      manualDataAmount = length / 5 + 1;
    }
  }
}