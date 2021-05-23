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


/*! @brief broadcast a message to all connected WebSocket clients
 * 
 * @param text the text to trasmit
 * @param justActive default: false, whether to just send it to active clients (in the app)
 * @param del default: 0, delay between individual messages
 * @param noPrint default: false, whether to avoid printnig the message to Serial
*/
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

/*! @brief broadcasting binary data to multiple devices
 * 
 * @param data pointer to a region in memory to send
 * @param length number of bytes to send
 * @param justActive whether to just send it to active (aka non-standby) devices
 * @param del delay in ms between individual messages
*/
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

/*! @brief clearing data after a client disconnects
 * 
 * @param spot the client that disconnected
*/
void removeClient (int spot) {
  #ifdef PRINT_WEBSOCKET_CONNECTIONS
    Serial.printf("[%u] Disconnected!\n", spot);
  #endif
  if (clients[spot][1] == 1) telemetryClientsCounter--;
  clients[spot][0] = 0;
  clients[spot][1] = 0;
  clientsCounter--;
}

/*! @brief routing after a client connects
 * 
 * sends current status to the client, LEDs, blocked UI elements and so on
 * @param spot the spot to send current status to
*/
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

/*! @brief send a message to a WebSocket client
 * 
 * @param spot the spot to send the message to, 255 for Serial
 * @param text the text to send
*/
void sendWSMessage(uint8_t spot, String text){
  if (spot != 255){
    webSocket.sendTXT(spot, text);
  } else {
    sPrintln(text);
  }
}

/*! @brief reconnect to wifi
 * 
 * disables ESC telemtry while reconnecting
*/
void reconnect() {
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
  Serial2.begin(115200);
}

/*! @brief checks the LED status
 *
 * transmits new LED status to connected clients if neccessary
*/
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

/*! @brief handles the wifi loop
 * 
 * runs WebSocket loop, checks LEDs and regularly sends telemtry to connected devices
*/
void handleWiFi() {
  webSocket.loop();
  checkLEDs();
  if (millis() > lastTelemetry + TELEMETRY_UPDATE_MS + TELEMETRY_UPDATE_ADD * telemetryClientsCounter) {
    lastTelemetry = millis();
    sendTelemetry();
  }
}