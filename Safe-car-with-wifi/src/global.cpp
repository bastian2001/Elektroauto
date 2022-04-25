#include "global.h"

//button variables
ButtonEvent lastButtonEvent;
unsigned long lastButtonDown = 0;

// status LED
int statusLED;


// EEPROM
bool commitFlag = false;

// control variables
int reqValue = 0;

// race mode, adjust LOG_SIZE when changing logged data
bool raceMode = false;
uint32_t raceStopAt;
bool automaticRace = true;

// WiFi/WebSockets
WebSocketsServer webSocket = WebSocketsServer(80);
uint8_t clients[MAX_WS_CONNECTIONS][2];
uint8_t telemetryClientsCounter = 0;

// ESCs
ESC *ESCs[2];

//light sensor
int * lsStatePtr;

bool sendESC = false;
uint8_t escPassthroughMode = 0;


String toBePrinted = "";

/** 
 * @brief Print the current Serial string
 */
void printSerial() {
  Serial.print(toBePrinted);
  toBePrinted = "";
}

/** 
 * @brief add anything to the Serial string
 * @param s the String to print
 */
void sPrint(String s) {
  toBePrinted += s;
}

/** 
 * @brief add a line to the Serial string
 * @param s the String to print
 */
void sPrintln(String s) {
  toBePrinted += s;
  toBePrinted += "\n";
}