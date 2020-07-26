#include <Arduino.h>
#include "WebSocketsServer.h"

void broadcastWSMessage(String text, bool justActive = false, int del = 0, bool noPrint = false);
void broadcastWSBin(uint8_t* data, size_t length, bool justActive = false, int del = 0);
void onWebSocketEvent(uint8_t clientNo, WStype_t type, uint8_t * payload, size_t length);
void removeClient (int spot);
void addClient (int spot);
void reconnect();
void handleWiFi();