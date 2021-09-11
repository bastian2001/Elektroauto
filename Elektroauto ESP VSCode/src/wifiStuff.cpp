#include "global.h"
#include "wifiStuff.h"
#include "messageHandler.h"
#include "system.h"

unsigned long lastTelemetry = 0;
char telData[24];
//armed, throttle0, throttle1, speed0, speed1, rps0, rps1, temp0, temp1, tempBMI, voltage0, voltage1, acceleration, raceModeThing
// 1      2          2          2       2       2     2     1      1      2        2         2         2             1            = 24

void broadcastWSMessage(String text, bool justActive, int del, bool noPrint){
  #ifdef PRINT_BROADCASTS
    uint8_t noOfDevices = 0;
  #endif
  for (int i = 0; i < MAX_WS_CONNECTIONS; i++) {
    if (clients[i][0] == 1 && (!justActive || clients[i][1] == 1)) {
      webSocket.sendTXT(i, text);
      #ifdef PRINT_BROADCASTS
        noOfDevices++;
      #endif
      if (del != 0){
        delay(del);
      }
    }
  }
  #ifdef PRINT_BROADCASTS
    if (noOfDevices > 0 && !noPrint){
      sPrint("Broadcasted ");
      sPrint(text);
      sPrint(" to a total of ");
      sPrint(String(noOfDevices));
      sPrintln(" devices.");
    }
  #endif
}

void broadcastWSBin(uint8_t* data, size_t length, bool justActive, int del){
  for (int i = 0; i < MAX_WS_CONNECTIONS; i++) {
    if (clients[i][0] == 1 && (!justActive || clients[i][1] == 1)) {
      webSocket.sendBIN(i, data, length);
      if (del != 0){
        delay(del);
      }
    }
  }
}

void onWebSocketEvent(uint8_t clientNo, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      removeClient(clientNo);
      break;
    case WStype_CONNECTED:
      addClient(clientNo);
      break;
    case WStype_TEXT:
      processMessage((char*)payload, clientNo);
      break;
    default:
      break;
  }
}

void removeClient (int spot) {
  #ifdef PRINT_WEBSOCKET_CONNECTIONS
    Serial.printf("[%u] Disconnected!\n", spot);
  #endif
  if (clients[spot][1] == 1) telemetryClientsCounter--;
  clients[spot][0] = 0;
  clients[spot][1] = 0;
}

void addClient (int spot) {
  if (spot < MAX_WS_CONNECTIONS && !raceActive) {
    if (!(ESCs[0]->status & ARMED_MASK) && !raceMode){
      webSocket.sendTXT(spot, "BLOCK VALUE 0");
    } else if (raceMode){
      webSocket.sendTXT(spot, "SET RACEMODE ON");
    }
    webSocket.sendTXT(spot, String("SET REDLED ") + ESCs[0]->getRedLED());
    webSocket.sendTXT(spot, String("SET GREENLED ") + ESCs[0]->getGreenLED());
    webSocket.sendTXT(spot, String("SET BLUELED ") + ESCs[0]->getBlueLED());
    String modeText = "SET MODESPINNER ";
    modeText += ctrlMode;
    webSocket.sendTXT(spot, modeText);
    webSocket.sendTXT(spot, String("MAXVALUE ") + String(getMaxValue(ctrlMode)));
    #ifdef PRINT_WEBSOCKET_CONNECTIONS
      IPAddress ip = webSocket.remoteIP(spot);
      Serial.printf("[%u] Connection from ", spot);
      Serial.println(ip.toString());
    #endif
  } else {
    #ifdef PRINT_WEBSOCKET_CONNECTIONS
      Serial.println(F("Connection refused. All spots filled!"));
    #endif
    webSocket.disconnect(spot);
  }
}

void sendWSMessage(uint8_t spot, String text){
  if (spot != 255){
    webSocket.sendTXT(spot, text);
  } else {
    sPrintln(text);
  }
}

void reconnect() {
  Serial1.end();
  WiFi.disconnect();
  while (WiFi.status() == WL_CONNECTED) {
    yield();
  }
  #ifdef PRINT_SETUP
    Serial.print("Reconnecting");
  #endif
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    #ifdef PRINT_SETUP
      Serial.println();
      Serial.println("WiFi-Connection could not be established!");
      Serial.println("Restart...");
    #endif
    ESP.restart();
  }
  #ifdef PRINT_SETUP
    Serial.println();
  #endif
  Serial1.begin(115200, SERIAL_8N1, ESC1_INPUT_PIN);
}

void sendTelemetry() {
  //armed, throttle0, throttle1, speed0, speed1, rps0, rps1, temp0, temp1, tempBMI, voltage0, voltage1, acceleration, raceModeThing
  // 1      2          2          2       2       2     2     1      1      2        2         2         2             1            = 24
  float rps0 = erpmToRps (ESCs[0]->heRPM);
  float rps1 = erpmToRps (ESCs[1]->heRPM);
  int slipPercent = 0;
  uint16_t speedWheel = ((float)(ESCs[0]->heRPM)) * erpmToMMPerSecond;
  if (speedWheel != 0) {
    slipPercent = (float)(speedWheel - speedBMI) / speedWheel * 100;
  }
  telData[0] = ESCs[0]->status & ARMED_MASK;
  telData[1] = ((uint16_t)ESCs[0]->currentThrottle) >> 8;
  telData[2] = ((uint16_t)ESCs[1]->currentThrottle) & 0xFF;

  // snprintf(telData, 73, "TELEMETRY a%d!p%d!u%d!t%d!u%d!r%d!s%d!v%d!w%d!c%d!%c%d", 
  //   (ESCs[0]->status & ARMED_MASK) > 0, (ESCs[0]->temperature + ESCs[1]->temperature) / 2, (ESCs[0]->voltage + ESCs[1]->voltage) / 2, (int) (ESCs[0]->currentThrottle), (int) (ESCs[1]->currentThrottle), (int) rps0, (int) rps1, slipPercent, (int) speedBMI, speedWheel, (int)(acceleration + .5), (raceMode && !raceActive) ? 'o' : 'q', reqValue);

  
  broadcastWSMessage(telData, true, 0, true);
}

void handleWiFi() {
  int i = 0;
  for (; i < 10; i++){
    if (broadcastQueue[i].isEmpty())
      break;
    broadcastWSMessage(broadcastQueue[i]);
    broadcastQueue[i] = "";
  }
  webSocket.loop();
  if (millis() > lastTelemetry + TELEMETRY_UPDATE_MS + TELEMETRY_UPDATE_ADD * telemetryClientsCounter) {
    lastTelemetry = millis();
    sendTelemetry();
  }
}