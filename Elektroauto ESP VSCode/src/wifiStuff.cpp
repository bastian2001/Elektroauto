#include "global.h"
#include "wifiStuff.h"
#include "telemetry.h"
#include "messageHandler.h"
#include "system.h"

unsigned long lastTelemetry = 0;

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
    if (!armed && !raceMode){
      webSocket.sendTXT(spot, "BLOCK VALUE 0");
    } else if (raceMode){
      webSocket.sendTXT(spot, "SET RACEMODE ON");
    }
    webSocket.sendTXT(spot, "SET REDLED " + String(redLED));
    webSocket.sendTXT(spot, "SET BLUELED " + String(blueLED));
    webSocket.sendTXT(spot, "SET GREENLED " + String(greenLED));
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

void checkLEDs(){
  if (newRedLED){
    redLED = newRedLED - 1;
    broadcastWSMessage("SET REDLED " + String(redLED));
    newRedLED = 0;
  }
  if (newGreenLED){
    greenLED = newGreenLED - 1;
    broadcastWSMessage("SET GREENLED " + String(greenLED));
    newGreenLED = 0;
  }
  if (newBlueLED){
    blueLED = newBlueLED - 1;
    broadcastWSMessage("SET BLUELED " + String(blueLED));
    newBlueLED = 0;
  }
}

void sendTelemetry() {
  float rps = erpmToRps (telemetryERPM);
  int slipPercent = 0;
  if (speedWheel != 0) {
    slipPercent = (float)(speedWheel - speedBMI) / speedWheel * 100;
  }
  char telData[66];
  snprintf(telData, 66, "TELEMETRY a%d!p%d!u%d!t%d!r%d!s%d!v%d!w%d!c%d!%c%d", 
    armed > 0, telemetryTemp, telemetryVoltage, (int) throttle, (int) rps, slipPercent, (int) speedBMI, speedWheel,
    (int)(acceleration + .5), (raceMode && !raceActive) ? 'o' : 'q', reqValue);
  broadcastWSMessage(telData, true, 0, true);
}

void handleWiFi() {
  webSocket.loop();
  checkLEDs();
  if (millis() > lastTelemetry + TELEMETRY_UPDATE_MS + TELEMETRY_UPDATE_ADD * telemetryClientsCounter) {
    lastTelemetry = millis();
    sendTelemetry();
  }
}