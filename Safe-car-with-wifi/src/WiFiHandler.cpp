#include "global.h"
#include "WiFiHandler.h"
#include "messageHandler.h"
#include "system.h"
#include "LED.h"
#include "lightSensor.h"

unsigned long lastTelemetry = 0;
uint8_t telData[31];
//armed, throttle0, throttle1, speed0, speed1, speedBMI, rps0, rps1, temp0, temp1, tempBMI, voltage0, voltage1, acceleration, raceModeThing, reqValue
// 1      2          2          2       2       2         2     2     1      1      2        2         2         2             1              2        = 28


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
  if (spot < MAX_WS_CONNECTIONS && !raceStopAt) {
    if (!(ESCs[0]->armed) && !raceMode){
      webSocket.sendTXT(spot, "BLOCK VALUE 0");
    } else if (raceMode){
      webSocket.sendTXT(spot, "SET RACEMODE ON");
    }
    webSocket.sendTXT(spot, String("MAXVALUE ") + String(getMaxValue()));
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
  WiFi.disconnect();
  WiFi.enableSTA(false);
  while (WiFi.status() == WL_CONNECTED) {
    yield();
    Serial.println();
  }
  WiFi.enableSTA(true);
  WiFi.setTxPower(WIFI_POWER_13dBm);
  WiFi.mode(WIFI_STA);
  #ifdef PRINT_SETUP
    Serial.println("Reconnecting");
  #endif
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    #ifdef PRINT_SETUP
      Serial.println("WiFi error!");
    #endif
  } else {
  #ifdef PRINT_SETUP
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  #endif
  }
}

int x = 0;
void sendTelemetry() {
	if (x == 0){
	for (int i = 0; i < 64; i++){
		Serial.print(ESCs[0]->items[i].duration0);
		Serial.print(' ');
		Serial.print(ESCs[0]->items[i].level0);
		Serial.print(' ');
		Serial.print(ESCs[0]->items[i].duration1);
		Serial.print(' ');
		Serial.print(ESCs[0]->items[i].level1);
		Serial.print(' ');
		if (ESCs[0]->items[i].duration0 == 0 && ESCs[0]->items[i].duration1 ==0) break;
	}
	Serial.println();
	}
	x++;
	if (x == 10) x = 0;
  uint16_t t0 = ESCs[0]->currentThrottle;
  uint16_t t1 = ESCs[1]->currentThrottle;
  telData[0] = ESCs[0]->armed;
  telData[1] = (t0) >> 8; //throttle
  telData[2] = (t0) & 0xFF;
  telData[3] = (t1) >> 8;
  telData[4] = (t1) & 0xFF;
  telData[11] = ESCs[0]->eRPM >> 8; // revolutions per second
  telData[12] = ESCs[0]->eRPM & 0xFF;
  telData[13] = ESCs[1]->eRPM >> 8;
  telData[14] = ESCs[1]->eRPM & 0xFF;
  telData[15]= ESCs[0]->arb;
  telData[16] = temperatureRead();
  // telData[19] = raceMode && !raceStopAt;//UI
  telData[20] = reqValue >> 8;
  telData[21] = reqValue & 0xFF;
  telData[22] = *lsStatePtr; // light sensor state
  telData[25] = ESCs[0]->sendFreq >> 8;
  telData[26] = ESCs[0]->sendFreq & 0xFF;
  telData[27] = ESCs[0]->loopFreq >> 8;
  telData[28] = ESCs[0]->loopFreq & 0xFF;
  if (ESCs[0]->loopFreq > 0xFFFF){
    telData[27] = 0xFF;
    telData[28] = 0xFF;
  }
  telData[29] = ESCs[0]->erpmFreq >> 8;
  telData[30] = ESCs[0]->erpmFreq & 0xFF;

  broadcastWSBin(telData, 31, true, 0);
}

void handleWiFi() {
  if (WiFi.status() != WL_CONNECTED){
    setStatusLED(LED_NO_WIFI);
    return;
  }
  if (statusLED == LED_NO_WIFI) resetStatusLED();
  if (webSocket.connectedClients() == 0)
    setStatusLED(LED_NO_DEVICE);
  else if (statusLED == LED_NO_DEVICE)
    resetStatusLED();
  webSocket.loop();
  if (millis() > lastTelemetry + TELEMETRY_UPDATE_MS + TELEMETRY_UPDATE_ADD * telemetryClientsCounter) {
    lastTelemetry = millis();
    sendTelemetry();
  }
}