#include "global.h"
#include "wifiStuff.h"
#include "telemetry.h"
#include "messageHandler.h"
#include "escIR.h"

unsigned long lastTelemetry = 0;
extern uint8_t telemetryClientsCounter;
uint8_t clientsCounter = 0;
extern WebSocketsServer webSocket;
extern bool raceMode, raceActive;
extern uint8_t clients[MAX_WS_CONNECTIONS][2];
extern bool armed;
extern int ctrlMode;

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
      Serial.print("Broadcasted ");
      Serial.print(text);
      Serial.print(" to a total of ");
      Serial.print(noOfDevices);
      Serial.println(" devices.");
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
      dealWithMessage((char*)payload, clientNo);
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
  clientsCounter--;
}

void addClient (int spot) {
  if (spot < MAX_WS_CONNECTIONS && !raceActive) {
    clientsCounter++;
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

void reconnect() {
  // detachInterrupt(digitalPinToInterrupt(ESC_TRIGGER_PIN));
  Serial2.end();
  WiFi.disconnect();
  int counterWiFi = 0;
  while (WiFi.status() == WL_CONNECTED) {
    yield();
  }
  #ifdef PRINT_SETUP
    Serial.print("Reconnecting");
  #endif
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
    if (counterWiFi == 30) {
      #ifdef PRINT_SETUP
        Serial.println();
        Serial.println("WiFi-Connection could not be established!");
      #endif
      break;
    }
    counterWiFi++;
  }
  #ifdef PRINT_SETUP
    Serial.println();
  #endif
  // attachInterrupt(digitalPinToInterrupt(ESC_TRIGGER_PIN), escir, RISING);
  Serial2.begin(115200);
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

void handleWiFi() {
  webSocket.loop();
  checkLEDs();
  if (millis() > lastTelemetry + TELEMETRY_UPDATE_MS + TELEMETRY_UPDATE_ADD * telemetryClientsCounter) {
    lastTelemetry = millis();
    sendTelemetry();
  }
}