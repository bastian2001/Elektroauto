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
#include "ArduinoJson.h"

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
float distMPU, speedMPU, acceleration;
int counterMPU, raw_accel, MPUoffset;
unsigned long lastMPUUpdate;

//rps control
double pidMulti, rpsA, rpsB, rpsC;
int previousRPS[TREND_AMOUNT], targetRPS;

//webSocket
WebSocketsServer webSocket = WebSocketsServer(80);
uint8_t clients[MAX_WS_CONNECTIONS][2];

//esc
int escOutputCounter, escOutputCounter2, escOutputCounter3;
uint16_t escValue;
uint16_t telemetryERPM, telemetryVoltage;
uint8_t telemetryTemp;

//raceMode
bool raceMode, raceActive, raceModeSendValues;
uint16_t logPosition;
uint16_t throttle_log[LOG_FRAMES], rps_log[LOG_FRAMES], voltage_log[LOG_FRAMES];
int acceleration_log[LOG_FRAMES];
uint8_t temp_log[LOG_FRAMES];

//system
bool armed, newValueFlag;
int ctrlMode, reqValue;
double throttle;


/*======================================================functional methods================================================*/

void sendRaceLog(){
  raceModeSendValues = false;
  Serial2.end();
  Serial.println("sendRaceLog triggered");
  for (uint16_t i = 0; i < LOG_FRAMES; i += 100){
    Serial.print("package no. ");
    Serial.println(i / 100);
    DynamicJsonDocument doc = DynamicJsonDocument(10000);
    doc["firstIndex"] = i;
    JsonArray JsonThrottleArray = doc.createNestedArray("throttle");
    JsonArray JsonAccelerationArray = doc.createNestedArray("acceleration");
    JsonArray JsonRPSArray = doc.createNestedArray("rps");
    JsonArray JsonVoltageArray = doc.createNestedArray("voltage");
    JsonArray JsonTemperatureArray = doc.createNestedArray("temperature");
    for (uint8_t i2 = 0; i2 < 100; i2++){
      JsonThrottleArray.add(throttle_log[i + i2]);
      JsonAccelerationArray.add(acceleration_log[i + i2]);
      JsonRPSArray.add(rps_log[i + i2]);
      JsonVoltageArray.add(voltage_log[i + i2]);
      JsonTemperatureArray.add(temp_log[i + i2]);
    }
    delay(1);
    String JsonOutput = "";
    serializeJson(doc, JsonOutput);
    #ifdef PRINT_RACE_MODE_JSON
      serializeJson(doc, Serial);
    #endif
    broadcastWSMessage(JsonOutput, true, 5, true);
    delay(10);
  }
  broadcastWSMessage("SET RACEMODETOGGLE OFF");
  Serial2.begin(115200);
}





/*======================================================system methods====================================================*/

void loop0() {
  if (WiFi.status() != WL_CONNECTED) {
    reconnect();
  }
  if (raceModeSendValues){
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

void setupVariables(){
  telemetryERPM = 0;
  telemetryTemp = 0;
  telemetryVoltage = 0;
  newValueFlag = false;

  telemetryClientsCounter = 0;

  distMPU = 0;
  speedMPU = 0;
  acceleration = 0;
  counterMPU = 0;
  lastMPUUpdate = 0;
  raw_accel = 0;
  MPUoffset = 0;

  targetRPS = 0;
  pidMulti = 7;
  rpsA = 0.00000008;
  rpsB = 0.000006;
  rpsC = 0.01;

  throttle = 0;
  armed = false;
  escValue = 0;
  ctrlMode = 0;
  reqValue = 0;

  escOutputCounter = 0;
  escOutputCounter2 = 0;
  escOutputCounter3 = 0;

  raceActive = false;
  raceModeSendValues = false;
  raceMode = false;
  logPosition = 0;
}

void setup() {

  setupVariables();

  //Serial setup
  Serial.begin(115200);
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

  //previousRPS Setup
  for (int i = 0; i < TREND_AMOUNT; i++) {
    previousRPS[i] = 0;
  }

  //2nd core setup
  xTaskCreatePinnedToCore( Task1code, "Task1", 10000, NULL, 0, &Task1, 0);

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
  #ifdef PRINT_TELEMETRY_RPS
    Serial.print("RPS\t");
  #endif
  #ifdef PRINT_TELEMETRY_ERPM
    Serial.print("ERPM\t");
  #endif
  #if defined(PRINT_TELEMETRY_THROTTLE) || defined(PRINT_TELEMETRY_TEMP) || defined(PRINT_TELEMETRY_VOLTAGE) || defined(PRINT_TELEMETRY_RPS) || defined(PRINT_TELEMETRY_ERPM)
    Serial.println();
  #endif
  c1ready = true;
  lastMPUUpdate = millis();
}