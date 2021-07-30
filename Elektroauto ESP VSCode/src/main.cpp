/// @file main.cpp the entry point for the project


/*====================================================includes====================================================*/

#include "global.h"
#include "system.h"
#include "wifiStuff.h"
#include "escFunctions.h"
#include "settings.h"
#include "telemetry.h"
#include "accelerometerFunctions.h"


/*====================================================global variables================================================*/

//Core 0 variables
TaskHandle_t core0Task;
bool c1ready = false, c0ready = false;

hw_timer_t * timer = NULL;

volatile unsigned long lastCore0 = 0;
volatile unsigned long lastCore1 = 0;



void onESCError(ESC * esc, uint8_t err){
  int pos = 0;
  while (!broadcastQueue[pos].isEmpty())
    pos++;
  switch(err){
    case ERROR_TOO_FAST:
      broadcastQueue[pos] = "MESSAGEBEEP Wegen Überhitzung disarmed";
      ESCs[0]->arm(false);
      ESCs[1]->arm(false);
      break;
    case ERROR_VOLTAGE_LOW:
      broadcastQueue[pos] = "MESSAGEBEEP Cutoff-Spannung unterschritten";
      ESCs[0]->arm(false);
      ESCs[1]->arm(false);
      break;
    case ERROR_OVERHEAT:
      broadcastQueue[pos] = "MESSAGEBEEP Zu hohe Drehzahl";
      ESCs[0]->arm(false);
      ESCs[1]->arm(false);
      break;
  }
}
void onStatusChange(ESC * esc, uint8_t newStatus, uint8_t oldStatus){
  int pos = 0;
  while (!broadcastQueue[pos].isEmpty())
    pos++;
  oldStatus ^= newStatus;
  if (oldStatus & CONNECTED_MASK){
    if (newStatus & CONNECTED_MASK){
      if (esc == ESCs[0])
        Serial.println("Verbindung zum linken ESC hergestellt");
      if (esc == ESCs[1])
        Serial.println("Verbindung zum rechten ESC hergestellt");
    } else {
      if (esc == ESCs[0])
        Serial.println("Verbindung zum linken ESC verloren");
      if (esc == ESCs[1])
        Serial.println("Verbindung zum rechten ESC verloren");
    }
  }
  if (oldStatus & RED_LED_MASK){
    broadcastQueue[pos++] = String("SET REDLED ") + esc->getRedLED();
  }
  if (oldStatus & GREEN_LED_MASK){
    broadcastQueue[pos++] = String("SET GREENLED ") + esc->getGreenLED();
  }
  if (oldStatus & BLUE_LED_MASK){
    broadcastQueue[pos++] = String("SET BLUELED ") + esc->getBlueLED();
  }
}





/*====================================================system methods==================================================*/

/**
 * @brief loop on core 0
 * 
 * the not so time-sensitive stuff<br>
 * initiates:
 * - wifi reconnection if neccessary
 * - sending race mode values
 * - low voltage checking
 * - User I/O handling
 * - Serial string printing
 */
void loop0() {
  if (WiFi.status() != WL_CONNECTED) {
    disableCore0WDT();
    lastCore0 = millis() + 5000;
    reconnect();
    delay(1);
    enableCore0WDT();
  }
  if (raceModeSendValues){
    raceMode = false;
    lastCore0 = millis() + 2000;
    broadcastWSMessage("SET RACEMODETOGGLE OFF");
    sendRaceLog();
    logPosition = 0;
  }
  checkVoltage();

  handleWiFi();
  receiveSerial();

  printSerial();
  delay(1);
  lastCore0 = millis();
  if (lastCore1 + (timerAlarmEnabled(timer) ? 5 : 500) < millis() && lastCore1 != 0 && millis() > 5000){
    Serial.println(String("Restarting because of Core 1 ") + lastCore0);
    // ESP.restart();
  }
}

/**
 * @brief loop on core 1
 * 
 * time sensitive stuff<br>
 * initiates:
 * - telemetry acquisition and processing
 * - throttle routine
 * - if neccessary, saves the settings
 */
void loop() {
  ESCs[0]->loop();
  ESCs[1]->loop();
  throttleRoutine();
  if (commitFlag){
    timerAlarmDisable(timer);
    commitFlag = false;
    EEPROM.commit();
    timerAlarmEnable(timer);
  }
  if (calibrateFlag){
    timerAlarmDisable(timer);
    calibrateAccelerometer();
    calibrateFlag = false;
    timerAlarmEnable(timer);
  }
  lastCore1 = millis();
  if (lastCore0 + 200 < millis() && lastCore0 != 0 && millis() > 5000){
    // Serial.println(String("Restarting because of Core 0 ") + lastCore0);
    // ESP.restart();
  }
}

//! @brief Task for core 0, creates loop0
void core0Code( void * parameter) {
  c0ready = true;
  while (!c1ready) {
    delay(1);
    lastCore0 = millis();
  }
  while (true) {
    loop0();
  }
}

/**
 * @brief setup function
 * 
 * - enables Serial communication
 * - connects to wifi
 * - sets up pins and timer for escir
 * - creates task on core 0
 * - initates websocket server
 */
void setup() {
  delay(100);

  //reading settings from EEPROM or writing them
  EEPROM.begin(52);
  if (firstStartup())
    writeEEPROM();
  else
    readEEPROM();

  //logData initialization
  logData = (uint16_t *)malloc(LOG_SIZE);
  throttle_log = logData + 0 * LOG_FRAMES;
  acceleration_log = (int16_t *)logData + 2 * LOG_FRAMES;
  erpm_log = logData + 4 * LOG_FRAMES;
  voltage_log = logData + 6 * LOG_FRAMES;
  temp_log = (uint8_t*)logData + 8 * LOG_FRAMES;
  // temp2_log = logData + 9 * LOG_FRAMES;

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
  delay(20);

  //BMI initialization
  initBMI();

  //ESC pins Setup
  pinMode(ESC1_OUTPUT_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  #if TRANSMISSION_IND != 0
  pinMode(TRANSMISSION_PIN, OUTPUT);
  digitalWrite(TRANSMISSION_PIN, HIGH);
  #endif
  ESCs[0] = new ESC(&Serial1, ESC1_OUTPUT_PIN, ESC1_INPUT_PIN, RMT_CHANNEL_0, onESCError, onStatusChange);
  ESCs[1] = new ESC(&Serial2, ESC2_OUTPUT_PIN, ESC2_INPUT_PIN, RMT_CHANNEL_1, onESCError, onStatusChange);
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &escIR, true);
  timerAlarmWrite(timer, ESC_FREQ, true);
  timerAlarmEnable(timer);

  //2nd core setup
  xTaskCreatePinnedToCore( core0Code, "core0Task", 60000, NULL, 0, &core0Task, 0);

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

  #if TELEMETRY_DEBUG != 0
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
  #endif
  lastCore1 = millis();
  c1ready = true;
  while (!c0ready) {
    delay(1);
    lastCore1 = millis();
  }
}