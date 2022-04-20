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

void IRAM_ATTR broadcastWSMessage(String text, bool justActive, int del, bool noPrint){
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
  uint16_t rps0 = erpmToRps (ESCs[0]->eRPM);
  uint16_t rps1 = erpmToRps (ESCs[1]->eRPM);
  uint16_t speedWheel0 = ((float)(ESCs[0]->eRPM)) * erpmToMMPerSecond;
  uint16_t speedWheel1 = ((float)(ESCs[1]->eRPM)) * erpmToMMPerSecond;
  uint16_t t0 = ESCs[0]->currentThrottle;
  uint16_t t1 = ESCs[1]->currentThrottle;
  int acc = acceleration + .5;
  int sBMI = speedBMI;
  int dBMI = distBMI;
  telData[0] = ESCs[0]->status & ARMED_MASK; //armed
  telData[1] = (t0) >> 8; //throttle
  telData[2] = (t0) & 0xFF;
  telData[3] = (t1) >> 8;
  telData[4] = (t1) & 0xFF;
  telData[5] = speedWheel0 >> 8; // speed in mm per second
  telData[6] = speedWheel0 & 0xFF;
  telData[7] = speedWheel1 >> 8;
  telData[8] = speedWheel1 & 0xFF;
  telData[9] = sBMI >> 8; // speed of the accelerometer
  telData[10] = sBMI & 0xFF;
  telData[11] = rps0 >> 8; // revolutions per second
  telData[12] = rps0 & 0xFF;
  telData[13] = rps1 >> 8;
  telData[14] = rps1 & 0xFF;
  telData[15] = bmiTemp; //temps
  telData[16] = temperatureRead();
  telData[17] = acc >> 8; // acceleration
  telData[18] = acc & 0xFF;
  telData[19] = raceMode && !raceActive;//UI
  telData[20] = reqValue >> 8;
  telData[21] = reqValue & 0xFF;
  telData[22] = *lsStatePtr; // light sensor state
  telData[23] = (dBMI >> 8) & 0xFF; //distance travelled according to BMI
  telData[24] = dBMI & 0xFF;
  telData[25] = ESCs[0]->sendFreq >> 8;
  telData[26] = ESCs[0]->sendFreq & 0xFF;
  telData[27] = ESCs[0]->loopFreq >> 8;
  telData[28] = ESCs[0]->loopFreq & 0xFF;
  telData[29] = ESCs[0]->erpmFreq >> 8;
  telData[30] = ESCs[0]->erpmFreq & 0xFF;

  broadcastWSBin(telData, 31, true, 0);
}

void handleWiFi() {
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