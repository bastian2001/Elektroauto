/*TODO:
   (Durchschnitt der bisherigen Gaswerte)
   anpassen, wobei das erst später geht
   (Online-Interface)
   Schlupf in Code implementieren
   Logging
*/


/*======================================================includes======================================================*/

#include <Arduino.h>
#include "WiFi.h"
#include <WebSocketsServer.h>
#include "driver/rmt.h"
#include "math.h"
#include <esp_task_wdt.h>

#include "global.h"
#include "system.h"
#include "escSetup.h"
#include "wifiStuff.h"
#include "escIR.h"
#include "telemetry.h"


/*======================================================global variables==================================================*/

//Core 0 variables
TaskHandle_t core0Task;
bool c1ready = false;

hw_timer_t * timer = NULL;

//webSocket
WebSocketsServer webSocket = WebSocketsServer(80);
uint8_t clients[MAX_WS_CONNECTIONS][2];




/*======================================================system methods====================================================*/

void loop0() {
  if (WiFi.status() != WL_CONNECTED) {
    disableCore0WDT();
    reconnect();
    delay(1);
    enableCore0WDT();
  }
  if (raceModeSendValues){
    raceMode = false;
    broadcastWSMessage("SET RACEMODETOGGLE OFF");
    sendRaceLog();
    logPosition = 0;
  }
  checkVoltage();

  handleWiFi();
  receiveSerial();

  printSerial();
  delay(1);
}

void loop() {
  // if (escirFinished){
    // handleMPU();
    // escirFinished = false;
  // }
  getTelemetry();
  evaluateThrottle();
}

void core0Code( void * parameter) {
  Serial2.begin(115200);

  while (!c1ready) {
    yield();
  }

  while (true) {
    loop0();
  }
}

void setup() {
  //Serial setup
  Serial.begin(500000);

  //WiFi Setup
  WiFi.enableSTA(true);
  WiFi.setTxPower(WIFI_POWER_MINUS_1dBm);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  #ifdef PRINT_SETUP
    Serial.print("Connecting to "); Serial.println(ssid);
  #endif
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    #ifdef PRINT_SETUP
      Serial.println("WiFi-Connection could not be established!");
      Serial.println("Restart...");
    #endif
    ESP.restart();
  }
  #ifdef PRINT_SETUP
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  #endif

  //MPU initialization
  // initMPU();

  //ESC pins Setup
  pinMode(ESC_OUTPUT_PIN, OUTPUT);
  pinMode(TRANSMISSION, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(TRANSMISSION, HIGH);
  esc_init(0, ESC_OUTPUT_PIN);
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &escir, true);
  timerAlarmWrite(timer, ESC_FREQ, true);
  timerAlarmEnable(timer);

  //2nd core setup
  xTaskCreatePinnedToCore( core0Code, "core0Task", 50000, NULL, 0, &core0Task, 0);

  //WebSockets init
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);
  for (int i = 0; i < MAX_WS_CONNECTIONS; i++) {
    clients[i][0] = 0;
    clients[i][1] = 0;
  }

  //setup termination
  #ifdef PRINT_SETUP
    Serial.println("ready");
  #endif

  #ifdef PRINT_TELEMETRY_THROTTLE
    Serial.print("Throttle\t");
  #endif
  #ifdef PRINT_TELEMETRY_TEMP
    Serial.print("Temp\t");
  #endif
  #ifdef PRINT_TELEMETRY_VOLTAGE
    Serial.print("Voltage\t");
  #endif
  #ifdef PRINT_TELEMETRY_ERPM
    Serial.print("ERPM\t");
  #endif
  #if defined(PRINT_TELEMETRY_THROTTLE) || defined(PRINT_TELEMETRY_TEMP) || defined(PRINT_TELEMETRY_VOLTAGE) || defined(PRINT_TELEMETRY_ERPM)
    Serial.println();
  #endif
  c1ready = true;
  lastMPUUpdate = millis();
}