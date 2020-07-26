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
// #include "Wire.h"
//#include "I2Cdev.h"
//#include "MPU6050.h"
#include <WebSocketsServer.h>
#include "driver/rmt.h"
#include "math.h"

#include "global.h"
#include "system.h"
#include "escSetup.h"
#include "wifiStuff.h"
#include "escIR.h"
#include "telemetry.h"


/*======================================================global variables==================================================*/

//Core 0 variables
TaskHandle_t Task1;
bool c1ready = false;

//MPU
float distMPU = 0, speedMPU = 0, acceleration = 0;
int counterMPU = 0, raw_accel = 0, MPUoffset = 0;
unsigned long lastMPUUpdate = 0;

//webSocket
WebSocketsServer webSocket = WebSocketsServer(80);
uint8_t clients[MAX_WS_CONNECTIONS][2];




/*======================================================system methods====================================================*/

void loop0() {
  if (WiFi.status() != WL_CONNECTED) {
    reconnect();
  }
  if (raceModeSendValues){
    raceMode = false;
    broadcastWSMessage("SET RACEMODETOGGLE OFF");
    Serial.println("sfdailusda");
    sendRaceLog();
    logPosition = 0;
  }
  handleWiFi();
  receiveSerial();
  printSerial();
  getTelemetry();
}

void loop() {
  /*if (Serial2.available()){
    sPrintln(String(Serial2.read()));
    }*/
}

void Task1code( void * parameter) {
  Serial2.begin(115200);
  disableCore0WDT();
  attachInterrupt(digitalPinToInterrupt(ESC_TRIGGER_PIN), escir, RISING);

  while (!c1ready) {
    yield();
  }

  while (true) {
    loop0();
  }
}

void setup() {
  //Serial setup
  Serial.begin(115200);
  Serial.println(RPS_CONVERSION_FACTOR);

  disableCore0WDT();

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

  //ESC pins Setup
  pinMode(ESC_OUTPUT_PIN, OUTPUT);
  pinMode(ESC_TRIGGER_PIN, OUTPUT);
  pinMode(TRANSMISSION, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(TRANSMISSION, HIGH);
  esc_init(0, ESC_OUTPUT_PIN);
  ledcSetup(1, ESC_FREQ, 8);
  ledcAttachPin(ESC_TRIGGER_PIN, 1);
  ledcWrite(1, 127);

  //2nd core setup
  xTaskCreatePinnedToCore( Task1code, "Task1", 50000, NULL, 0, &Task1, 0);

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