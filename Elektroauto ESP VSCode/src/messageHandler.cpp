#include "global.h"
#include "messageHandler.h"
#include "wifiStuff.h"
#include "settings.h"
#include "system.h"
#include "accelerometerFunctions.h"

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
        value = MODE_RPS;
    } else if (valueStr == "SLIP"){
        value = MODE_SLIP;
    }
    ctrlMode = value;
    String modeText = "SET MODESPINNER ";
    modeText += ctrlMode;
    switch(ctrlMode){
      case MODE_THROTTLE:
        reqValue = nextThrottle; // stationary
        break;
      case MODE_RPS:
        if (erpmToRps(telemetryERPM) > maxTargetRPS)
          reqValue = maxTargetRPS;
        reqValue = erpmToRps(telemetryERPM);
        targetERPM = rpsToErpm(reqValue);
        break;
      case MODE_SLIP:
        if (reqValue > maxTargetSlip)
          reqValue = maxTargetSlip;
        targetSlip = reqValue;
        break;
    }
    broadcastWSMessage(modeText);
    broadcastWSMessage(String("MAXVALUE ") + String(getMaxValue(ctrlMode)));
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
    if (!message.endsWith("NOMESSAGE"))
      sendWSMessage(from, bcMessage);
  }
  else if (command == "WARNINGVOLTAGE"){
    warningVoltage = message.substring(dividerPos + 1).toInt();
    char bcMessage[50];
    snprintf(bcMessage, 50, "MESSAGE Spannungswarnung erfolgt unter %4.2fV", (double)warningVoltage/100);
    if (!message.endsWith("NOMESSAGE"))
      sendWSMessage(from, bcMessage);
  }
  else if (command == "RPSA"){
    erpmA = message.substring(dividerPos + 1).toFloat();
    if (!message.endsWith("NOMESSAGE"))
      sendWSMessage(from, String("MESSAGE Tempomat 3. Potenz ist nun ") + String(erpmA));
  }
  else if (command == "RPSB"){
    erpmB = message.substring(dividerPos + 1).toFloat();
    if (!message.endsWith("NOMESSAGE"))
      sendWSMessage(from, String("MESSAGE Tempomat 2. Potenz ist nun ") + String(erpmB));
  }
  else if (command == "RPSC"){
    erpmC = message.substring(dividerPos + 1).toFloat();
    if (!message.endsWith("NOMESSAGE"))
      sendWSMessage(from, String("MESSAGE Tempomat 1. Potenz ist nun ") + String(erpmC));
  }
  else if (command == "PIDMULTIPLIER"){
    pidMulti = message.substring(dividerPos + 1).toFloat();
    if (!message.endsWith("NOMESSAGE"))
      sendWSMessage(from, String("MESSAGE Tempomat-Master ist nun ") + String(pidMulti));
  }
  else if (command == "SLIPMULTIPLIER"){
    slipMulti = message.substring(dividerPos + 1).toFloat();
    if (!message.endsWith("NOMESSAGE"))
      sendWSMessage(from, String("MESSAGE Schlupf-Master ist nun ") + String(slipMulti));
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
    if (length % 5 != 4 || length > MAX_MANUAL_DATA * 5 - 1){
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
    maxThrottle = message.substring(dividerPos + 1).toInt();
    if (ctrlMode == MODE_THROTTLE)
      broadcastWSMessage(String("MAXVALUE ") + String(maxThrottle));
    if (!message.endsWith("NOMESSAGE"))
      sendWSMessage(from, String("MESSAGE Max. Gaswert ist nun ") + String(maxThrottle));
  }
  else if (command == "MAXR" || command == "MAXRPS"){
    maxTargetRPS = message.substring(dividerPos + 1).toInt();
    if (ctrlMode == MODE_RPS)
      broadcastWSMessage(String("MAXVALUE ") + String(maxTargetRPS));
    if (!message.endsWith("NOMESSAGE"))
      sendWSMessage(from, String("MESSAGE Max. Ziel-RPS ist nun ") + String(maxTargetRPS));
  }
  else if (command == "MAXS" || command == "MAXSLIP"){
    maxTargetSlip = message.substring(dividerPos + 1).toInt();
    if (ctrlMode == MODE_SLIP)
      broadcastWSMessage(String("MAXVALUE ") + String(maxTargetSlip));
    if (!message.endsWith("NOMESSAGE"))
      sendWSMessage(from, String("MESSAGE Max. Ziel-Schlupf ist nun ") + String(maxTargetSlip));
  }
  else if (command == "MOTORPOLE"){
    motorPoleCount = message.substring(dividerPos + 1).toInt();
    rpsConversionFactor = (1.6667f / ((float)motorPoleCount / 2.0f));
    erpmToMMPerSecond = (rpsConversionFactor * (float)wheelDiameter * PI);
    if (!message.endsWith("NOMESSAGE"))
      sendWSMessage(from, String("MESSAGE Anzahl Motorpole ist nun ") + String(motorPoleCount));
  }
  else if (command == "WHEELDIA" || command == "WHEELDIAMETER"){
    wheelDiameter = message.substring(dividerPos + 1).toInt();
    erpmToMMPerSecond = (rpsConversionFactor * (float)wheelDiameter * PI);
    if (!message.endsWith("NOMESSAGE"))
      sendWSMessage(from, String("MESSAGE Raddurchmesser beträgt nun ") + String(pidMulti) + String(" mm"));
  }
  
  // else if (command == "CALIBRATE"){ // currently results in crash
  //   calibrateFlag = true;
  //   if (!message.endsWith("NOMESSAGE"))
  //     sendWSMessage(from, "MESSAGE Beschleunigungssensor wird kalibriert");
  // }

  else if (command == "REBOOT" || command == "RESTART"){
    ESP.restart();
  }
}