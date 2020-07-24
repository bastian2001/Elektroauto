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



/*======================================================definitions======================================================*/

//Pin numbers
#define ESC_OUTPUT_PIN 25
#define ESC_TRIGGER_PIN 33
#define TRANSMISSION  23
#define LED_BUILTIN 22

//PID loop settings
#define TREND_AMOUNT 5 //nur ungerade!!
#define TA_DIV_2 2 //mit anpassen!!

//ESC values
#define ESC_FREQ 1000
#define ESC_BUFFER_ITEMS 16
#define CLK_DIV 3 //DShot 150: 12, DShot 300: 6, DShot 600: 3
#define TT 44 // total bit time
#define T0H 17 // 0 bit high time
#define T1H 33 // 1 bit high time
#define T0L (TT - T0H) // 0 bit low time
#define T1L (TT - T1H) // 1 bit low time
#define T_RESET 21 // reset length in multiples of bit time
#define ESC_TELEMETRY_REQUEST 0
#define TRANSMISSION_IND 1000
#define TELEMETRY_DEBUG 3
#define MAX_THROTTLE 350
// #define SEND_TRANSMISSION_IND

//logging settings
#define LOG_FRAMES 5000

//WiFi and WebSockets settings
#define ssid "KNS_WLAN_24G"
#define password "YZKswQHaE4xyKqdP"
// #define ssid "Coworking"
// #define password "86577103963855526306"
// #define ssid "bastian"
// #define password "hallo123"
#define MAX_WS_CONNECTIONS 5
#define TELEMETRY_UPDATE_MS 20
#define TELEMETRY_UPDATE_ADD 20

//debugging settings
#define PRINT_SETUP
// #define PRINT_TELEMETRY_THROTTLE
// #define PRINT_TELEMETRY_RPS
// #define PRINT_TELEMETRY_TEMP
// #define PRINT_TELEMETRY_ERPM
// #define PRINT_TELEMETRY_VOLTAGE
#define PRINT_WEBSOCKET_CONNECTIONS
#define PRINT_INCOMING_MESSAGES
#define PRINT_MEANINGS
#define PRINT_BROADCASTS
// #define PRINT_SINGLE_OUTGOING_MESSAGES
// #define PRINT_RACE_MODE_JSON



/*======================================================global variables==================================================*/

//Core 0 variables
TaskHandle_t Task1;
String toBePrinted = "";

//rps control variables
int targetRPS = 0;
int previousRPS[TREND_AMOUNT];
double pidMulti = 7;
double rpsA = .00000008;
double rpsB = .000006;
double rpsC = .01;

//MPU variables
//MPU6050 mpu;
int16_t MPUoffset = 0, raw_accel = 0;
unsigned long lastMPUUpdate = 0;
int counterMPU = 0;
float acceleration = 0, speedMPU = 0, distMPU = 0;

//system variables
double throttle = 0;
bool armed = false, c1ready = false, newReqValueForTelemetry = true;
int ctrlMode = 0, reqValue = 0;
uint16_t escValue = 0;
uint16_t errorCount = 0;
// uint16_t cutoffVoltage = 900;

//WiFi/WebSockets variables
WebSocketsServer webSocket = WebSocketsServer(80);
uint8_t clients[MAX_WS_CONNECTIONS][2]; //[device (disconnected, app, web, ajax)][telemetry (off, on)]
unsigned long lastTelemetry = 0;
uint8_t clientsCounter = 0, telemetryClientsCounter = 0;

//ESC variables
rmt_item32_t esc_data_buffer[ESC_BUFFER_ITEMS];
char escTelemetry[10];
uint16_t telemetryERPM = 0;
uint8_t telemetryTemp = 0;
uint16_t telemetryVoltage = 0;
bool newValueFlag = false;

//arbitrary variables
int16_t escOutputCounter = 0, escOutputCounter2 = 0, escOutputCounter3 = 0;

//race mode variables
uint16_t throttle_log[LOG_FRAMES], rps_log[LOG_FRAMES], voltage_log[LOG_FRAMES];
int16_t acceleration_log[LOG_FRAMES];
uint8_t temp_log[LOG_FRAMES];
uint16_t logPosition = 0;
bool raceModeSendValues = false, raceMode = false, raceActive = false;

void broadcastWSMessage(String text, bool justActive = false, int del = 0, bool noPrint = false);
void setArmed(bool arm, bool sendBroadcast = false);
void startRace();


/*======================================================functional methods================================================*/

void printSerial() {
  Serial.print(toBePrinted);
  toBePrinted = "";
}

void sPrint(String s) {
  toBePrinted += s;
}

void sPrintln(String s) {
  toBePrinted += s;
  toBePrinted += "\n";
}

void sendTelemetry() {
  int velMPU = (int)(speedMPU * 1000 + .5);
  int velWheel = (int)((float)30 * PI * (float)previousRPS[TREND_AMOUNT - 1]);
  int slipPercent = 0;
  if (velWheel != 0) {
    slipPercent = (float)(velWheel - velMPU) / velWheel * 100;
  }
  //String telemetryData2 = "";
  String telemetryData = armed ? "TELEMETRY a1!p" : "TELEMETRY a0!p";
  telemetryData += telemetryTemp;
  telemetryData += "!u";
  telemetryData += telemetryVoltage;
  telemetryData += "!t";
  telemetryData += ((int) throttle);
  telemetryData += "!r";
  telemetryData += previousRPS[TREND_AMOUNT - 1];
  telemetryData += "!s";
  telemetryData += slipPercent;
  telemetryData += "!v";
  telemetryData += velMPU;
  telemetryData += "!w";
  telemetryData += velWheel;
  telemetryData += "!c";
  telemetryData += ((int)(acceleration * 1000 + .5));
  if (newReqValueForTelemetry){
    telemetryData += (raceMode && !raceActive) ? "!o" : "!q";
    telemetryData += reqValue;
    newReqValueForTelemetry = false;
  }
  broadcastWSMessage(telemetryData, true, 0, true);
}

void esc_init(uint8_t channel, uint8_t pin) {
  rmt_config_t config;
  config.rmt_mode = RMT_MODE_TX;
  config.channel = ((rmt_channel_t) channel);
  config.gpio_num = ((gpio_num_t) pin);
  config.mem_block_num = 3;
  config.tx_config.loop_en = false;
  config.tx_config.carrier_en = false;
  config.tx_config.idle_output_en = true;
  config.tx_config.idle_level = ((rmt_idle_level_t) 0);
  config.clk_div = 6; // target: DShot 300 (300kbps)

  ESP_ERROR_CHECK(rmt_config(&config));
  ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
}

void setup_rmt_data_buffer(uint16_t value) {
  uint16_t mask = 1 << (ESC_BUFFER_ITEMS - 1);
  for (uint8_t bit = 0; bit < ESC_BUFFER_ITEMS; bit++) {
    uint16_t bit_is_set = value & mask;
    esc_data_buffer[bit] = bit_is_set ? (rmt_item32_t) {{{T1H, 1, T1L, 0}}} : (rmt_item32_t) {{{T0H, 1, T0L, 0}}};
    mask >>= 1;
  }
}

void esc_send_value(uint16_t value, bool wait) {
  setup_rmt_data_buffer(value);
  ESP_ERROR_CHECK(rmt_write_items((rmt_channel_t) 0, esc_data_buffer, ESC_BUFFER_ITEMS, wait));
}

uint16_t appendChecksum(uint16_t value) {
  value &= 0x7FF;
  escOutputCounter2 = (escOutputCounter2 == ESC_TELEMETRY_REQUEST) ? 0 : escOutputCounter2 + 1;
  bool telem = escOutputCounter2 == 0;
  value = (value << 1) | telem;
  int csum = 0, csum_data = value;
  for (int i = 0; i < 3; i++) {
    csum ^=  csum_data;   // xor data by nibbles
    csum_data >>= 4;
  }
  csum &= 0xf;
  value = (value << 4) | csum;
  return value;
}

void setThrottle(double newThrottle) { //throttle value between 0 and 2000 --> esc value between 0 and 2047 with checksum
  newThrottle = (newThrottle > 2000) ? 2000 : newThrottle;
  newThrottle = (newThrottle < 0) ? 0 : newThrottle;
  newThrottle = (newThrottle > MAX_THROTTLE) ? MAX_THROTTLE : newThrottle;
  throttle = newThrottle;
  newThrottle += (newThrottle == 0) ? 0 : 47;
  escValue = appendChecksum(newThrottle);
}

void setArmed (bool arm, bool sendBroadcast){
  if (arm != armed){
    if (!raceActive){
      reqValue = 0;
      targetRPS = 0;
    }
    broadcastWSMessage(arm ? "UNBLOCK VALUE" : "BLOCK VALUE 0");
    armed = arm;
    setThrottle(0);
  } else if (sendBroadcast){
    broadcastWSMessage(arm ? "MESSAGE already armed" : "MESSAGE already disarmed");
  }
}

void startRace(){
  if (!raceActive && raceMode){
    broadcastWSMessage("BLOCK RACEMODETOGGLE ON");
    raceActive = true;
    setArmed(true);
    newValueFlag = true;
  } else if (!raceMode) {
    broadcastWSMessage("SET RACEMODETOGGLE OFF");
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

void dealWithMessage(String message, uint8_t from) {
  #ifdef PRINT_INCOMING_MESSAGES
    Serial.print(F("Received: \""));
    Serial.print(message);
    Serial.println(F("\""));
  #endif
  int dividerPos = message.indexOf(":");
  String command = dividerPos == -1 ? message : message.substring(0, dividerPos);
  if (command == "VALUE" && dividerPos != -1){
    reqValue = message.substring(dividerPos + 1).toInt();
    newValueFlag = true;
    newReqValueForTelemetry = true;
  } else if (command == "ARMED" && dividerPos != -1){
    String valueStr = message.substring(dividerPos + 1);
    valueStr.toUpperCase();
    int value = valueStr.toInt();
    if (valueStr == "YES" || valueStr == "TRUE") value = 1;
    setArmed(value > 0, true);
  } else if (command == "DEVICE" && dividerPos != -1 && from != 255){
    String valueStr = message.substring(dividerPos + 1);
    int value = valueStr.toInt();
    if (valueStr == "APP") value = 1;
    clients[from][0] = value;
  } else if (command == "TELEMETRY" && dividerPos != -1 && from != 255){
    String valueStr = message.substring(dividerPos + 1);
    valueStr.toUpperCase();
    int value = valueStr.toInt();
    if (valueStr == "ON") value = 1;
    if (clients[from][1] > 0 && value == 0) telemetryClientsCounter--;
    if (clients[from][1] == 0 && value > 0) telemetryClientsCounter++;
    clients[from][1] = value;
    #ifdef PRINT_MEANINGS
      Serial.print("Setting telemetry of spot ");
      Serial.print(from);
      Serial.println(value > 0 ? " ON" : " OFF");
    #endif
  } else if (command == "MODE" && dividerPos != -1){
    String valueStr = message.substring(dividerPos + 1);
    valueStr.toUpperCase();
    int value = valueStr.toInt();
    if (valueStr == "RPS"){
        value = 1;
    } else if (valueStr == "SLIP"){
        value = 2;
    }
    ctrlMode = value;
    String modeText = "SET MODESPINNER ";
    modeText += ctrlMode;
    broadcastWSMessage(modeText);
    newValueFlag = true;
  } else if (command == "RACEMODE" && dividerPos != -1){
    String valueStr = message.substring(dividerPos + 1);
    valueStr.toUpperCase();
    bool raceModeOn = valueStr.toInt() > 0;
    if (valueStr == "ON") raceModeOn = 1;
    if (raceModeOn){
      broadcastWSMessage("SET RACEMODETOGGLE ON");
      broadcastWSMessage("UNBLOCK VALUE");
    } else {
      broadcastWSMessage("SET RACEMODETOGGLE OFF");
      if (!armed)
        broadcastWSMessage("BLOCK VALUE 0");
    }
    raceMode = raceModeOn;
  } else if (command == "STARTRACE"){
    startRace();
  } else if (command == "PING") {
    webSocket.sendTXT(from, "PONG");
  // } else if (command == "CUTOFFVOLTAGE"){
    // cutoffVoltage = message.substring(dividerPos + 1).toInt();
  } else if (command == "RPSA"){
    rpsA = message.substring(dividerPos + 1).toFloat();
  } else if (command == "RPSB"){
    rpsB = message.substring(dividerPos + 1).toFloat();
  } else if (command == "RPSC"){
    rpsC = message.substring(dividerPos + 1).toFloat();
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

double calcThrottle(int target, int was[]) {
  double was_avg = 0;
  int was_sum = 0, t_sq_sum = 0, t_multi_was_sum = 0;
  for (int i = 0; i < TREND_AMOUNT; i++) {
    int t = i - TA_DIV_2;
    was_sum += was[i];
    t_sq_sum += pow(t, 2);
    t_multi_was_sum += t * was[i];
  }
  was_avg = (double)was_sum / TREND_AMOUNT;

  double m = (double)t_multi_was_sum / (double)t_sq_sum;

  double prediction = m * ((double)TREND_AMOUNT - (double)TA_DIV_2) + (double)was_avg;
  if (prediction < 0) prediction = 0;
  double delta_rps = target - prediction;
  double delta_throttle = rpsA * pow(delta_rps, 3) + rpsB * pow(delta_rps, 2) + rpsC * delta_rps;
  delta_throttle *= pidMulti;

  throttle += delta_throttle;

  return throttle;
}

void escir() {
  for (int i = 0; i < TREND_AMOUNT - 1; i++) {
    previousRPS[i] = previousRPS[i + 1];
  }
  previousRPS[TREND_AMOUNT - 1] = (int) ((double) telemetryERPM / (double) 4.2); // *100/7(cause Erpm)/60(cause erpM)
  escOutputCounter3 = (escOutputCounter3 == TELEMETRY_DEBUG) ? 0 : escOutputCounter3 + 1;
  if (escOutputCounter3 == 0){ // print debug telemetry over Serial
    #ifdef PRINT_TELEMETRY_THROTTLE
      sPrint((String)((int)throttle));
      sPrint("\t");
    #endif
    #ifdef PRINT_TELEMETRY_TEMP
      sPrint((String)telemetryTemp);
      sPrint("\t");
    #endif
    #ifdef PRINT_TELEMETRY_VOLTAGE
      sPrint((String)telemetryVoltage);
      sPrint("\t");
    #endif
    #ifdef PRINT_TELEMETRY_RPS
      sPrint((String)previousRPS[TREND_AMOUNT - 1]);
      sPrint("\t");
    #endif
    #ifdef PRINT_TELEMETRY_ERPM
      sPrint((String)telemetryERPM);
      sPrint("\t");
    #endif
    #if defined(PRINT_TELEMETRY_THROTTLE) || defined(PRINT_TELEMETRY_TEMP) || defined(PRINT_TELEMETRY_VOLTAGE) || defined(PRINT_TELEMETRY_RPS) || defined(PRINT_TELEMETRY_ERPM)
      sPrintln("");
    #endif
  }

  if (armed) {
    if (newValueFlag) {
      //once when new value is given
      newValueFlag = false;
      switch (ctrlMode) {
        case 0:
          setThrottle(reqValue);
          break;
        case 1:
          targetRPS = reqValue;
          break;
        case 2:
          break;
        default:
          break;
      }
    }
    //every esc cycle: calculation of throttle if necessary
    switch (ctrlMode) {
      case 0:
        break;
      case 1:
        setThrottle(calcThrottle(targetRPS, previousRPS));
        break;
      case 2:
        break;
      default:
        break;
    }
  } else {
    setThrottle(0);
  }
  #ifdef SEND_TRANSMISSION_IND
  escOutputCounter = (escOutputCounter == TRANSMISSION_IND) ? 0 : escOutputCounter + 1;
  if (escOutputCounter == 0)
    digitalWrite(TRANSMISSION, LOW);
  delayMicroseconds(20);
  #endif
  esc_send_value(escValue, false);
  #ifdef SEND_TRANSMISSION_IND
  if (escOutputCounter == 0)
    digitalWrite(TRANSMISSION, HIGH);
  #endif
  if (raceActive){
    throttle_log[logPosition] = (uint16_t)(throttle + .5);
    acceleration_log[logPosition] = 0;
    rps_log[logPosition] = previousRPS[TREND_AMOUNT - 1];
    voltage_log[logPosition] = telemetryVoltage;
    temp_log[logPosition] = telemetryTemp;
    logPosition++;
    if (logPosition == LOG_FRAMES){
      sPrintln("sending race log now");
      raceActive = false;
      raceModeSendValues = true;
    }
  }
}

bool isTelemetryComplete(){
  return escTelemetry[3] == 0 && escTelemetry[4] == 0 && escTelemetry[5] == 0 && escTelemetry[6] == 0;
}

void getTelemetry(){
  while(Serial2.available()){
    for (uint8_t i = 0; i < 9; i++){
      escTelemetry[i] = escTelemetry[i+1];
    }
    escTelemetry[9] = (char) Serial2.read();
    if (isTelemetryComplete()){
      telemetryTemp = escTelemetry[0];
      telemetryVoltage = (escTelemetry[1] << 8) | escTelemetry[2];
      telemetryERPM = (escTelemetry[7] << 8) | escTelemetry[8];
      for (uint8_t i = 0; i < 9; i++){
        escTelemetry[i] = B1;
      }
      if (telemetryVoltage == 257/*telemetryVoltage < cutoffVoltag*/){
        // bool pArmed = armed;
        setArmed(false);
        // if (pArmed)
        //   broadcastWSMessage("MESSAGE Cutoff-Spannung unterschritten. Gerät disarmed", true);
      }
      if (telemetryTemp > 140 || telemetryVoltage > 2000 || telemetryVoltage < 500 || telemetryERPM > 2000){
        errorCount++;
        // Serial.println("error");
        break;
      }
      break;
    }
  }
}

void reconnect() {
  detachInterrupt(digitalPinToInterrupt(ESC_TRIGGER_PIN));
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
  attachInterrupt(digitalPinToInterrupt(ESC_TRIGGER_PIN), escir, RISING);
  Serial2.begin(115200);
}

void receiveSerial() {
  if (Serial.available()) {
    String readout = Serial.readStringUntil('\n');
    dealWithMessage(readout, 255);
    float val = readout.substring(1).toFloat();
    switch (readout.charAt(0)) {
      case 'p':
        pidMulti = val;
        break;
      case 'c':
        reconnect();
        break;
    }
  }
}

void handleWiFi() {
  webSocket.loop();
  if (millis() > lastTelemetry + TELEMETRY_UPDATE_MS + TELEMETRY_UPDATE_ADD * telemetryClientsCounter) {
    lastTelemetry = millis();
    sendTelemetry();
  }
}

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

void setup() {

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