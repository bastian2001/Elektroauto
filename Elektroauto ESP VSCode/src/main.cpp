/// @file main.cpp the entry point for the project


/*====================================================includes====================================================*/

#include "global.h"
#include "system.h"
#include "escIR.h"
#include "WiFiHandler.h"
#include "settings.h"
#include "accelerometerFunctions.h"
#include "LED.h"
#include "button.h"
#include "lightSensor.h"
#include "race.h"

/*====================================================global variables================================================*/
extern bool lightSensorStart;
//Core 0 variables
TaskHandle_t core0Task;
bool c1ready = false, c0ready = false;

hw_timer_t * timer = NULL;

const char ERROR_OVERHEAT_MESSAGE[] = "MESSAGEBEEP Wegen Überhitzung disarmed\0";
const char ERROR_CUTOFF_MESSAGE[] = "MESSAGEBEEP Cutoff-Spannung unterschritten\0";
const char ERROR_TOO_FAST_MESSAGE[] = "MESSAGEBEEP Zu hohe Drehzahl\0";

void onESCError(ESC * esc, uint8_t err){
  int pos = 0;
  while (actionQueue[pos].type != 0)
    pos++;
  Action a;
  switch(err){
    case ERROR_TOO_FAST: {
      a.type = 1; //disarm
      actionQueue[pos++] = a;
      a.type = 2; //send too fast message
      char * s = (char*)malloc(50);
      strcpy(s, ERROR_TOO_FAST_MESSAGE);
      a.payload = (int)s;
      actionQueue[pos] = a;
      break;
    }
    case ERROR_VOLTAGE_LOW: {
      a.type = 1; //disarm
      actionQueue[pos++] = a;
      a.type = 2; //send too fast message
      char * s = (char*)malloc(50);
      strcpy(s, ERROR_CUTOFF_MESSAGE);
      a.payload = (int)s;
      actionQueue[pos] = a;
      break;
    }
    case ERROR_OVERHEAT: {
      a.type = 1; //disarm
      actionQueue[pos++] = a;
      a.type = 2; //send too fast message
      char * s = (char*)malloc(50);
      strcpy(s, ERROR_OVERHEAT_MESSAGE);
      a.payload = (int)s;
      actionQueue[pos] = a;
      break;
    }
  }
}
void onStatusChange(ESC * esc, uint8_t newStatus, uint8_t oldStatus){
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
  Action a;
  if (oldStatus & RED_LED_MASK){
    String s = "SET REDLED " + String(esc->getRedLED());
    a.type = 2;
    char * str = (char *)malloc(20);
    s.toCharArray(str, 20);
    a.payload = (int)str;
    int pos = 0;
    while (actionQueue[pos].type != 0)
      pos++;
    actionQueue[pos] = a;
  }
  if (oldStatus & GREEN_LED_MASK){
    String s = "SET GREENLED " + String(esc->getGreenLED());
    a.type = 2;
    char * str = (char *)malloc(20);
    s.toCharArray(str, 20);
    a.payload = (int)str;
    int pos = 0;
    while (actionQueue[pos].type != 0)
      pos++;
    actionQueue[pos] = a;
  }
  if (oldStatus & BLUE_LED_MASK){
    String s = "SET BLUELED " + String(esc->getBlueLED());
    a.type = 2;
    char * str = (char *)malloc(20);
    s.toCharArray(str, 20);
    a.payload = (int)str;
    int pos = 0;
    while (actionQueue[pos].type != 0)
      pos++;
    actionQueue[pos] = a;
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
  if (lightSensorStart){
    lightSensorStart = false;
    startRace();
  }
  if (WiFi.status() != WL_CONNECTED) {
    disableCore0WDT();
    reconnect();
    delay(1);
    enableCore0WDT();
  }
  if (raceModeSendValues){
    raceMode = false;
    broadcastWSMessage("SET RACEMODETOGGLE OFF");
    if (finishFrame < LOG_FRAMES - 1)
      broadcastWSMessage(String("MESSAGE Rennzeit (s): ") + String((float)(finishFrame + 1) / (float) ESC_FREQ));
    else
      broadcastWSMessage("MESSAGE Ziel laut BMI nicht erreicht");
    sendRaceLog();
    logPosition = 0;
    if (statusLED >= LED_RACE_MODE && statusLED <= LED_RACE_ARMED_ACTIVE) resetStatusLED();
  }
  // checkVoltage();

  handleWiFi();
  receiveSerial();

  checkButton();
  ledRoutine();

  runActions();

  printSerial();

  if (commitFlag){
    timerAlarmDisable(timer);
    pauseLS();
    commitFlag = false;
    EEPROM.commit();
    timerAlarmEnable(timer);
    resumeLS();
  }
  if (calibrateFlag){
    timerAlarmDisable(timer);
    pauseLS();
    calibrateAccelerometer();
    calibrateFlag = false;
    timerAlarmEnable(timer);
    resumeLS();
  }

  delay(1);
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
  if (raceStartedAt && millis() - raceStartedAt > 4000){
    setArmed(false);
  }
  throttleRoutine();
}

//! @brief Task for core 0, creates loop0
void core0Code( void * parameter) {
  c0ready = true;
  while (!c1ready) {
    delay(1);
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
  pinMode(26, OUTPUT);
  digitalWrite(26, HIGH); // enable BMI160
  pinMode(LED_PIN, OUTPUT);
  setStatusLED(LED_SETUP);
  ledRoutine();
  //Serial setup
  Serial.begin(500000);

  delay(100);

  //reading settings from EEPROM or writing them
  EEPROM.begin(76);
  if (firstStartup())
    writeEEPROM();
  else
    readEEPROM();

  Serial.println();
  //logData initialization
  throttle_log0 = (uint16_t *)(logData + 256 + 0 * LOG_FRAMES);
  throttle_log1 = (uint16_t *)(logData + 256 + 2 * LOG_FRAMES);
  // erpm_log0 = (uint16_t *)(logData + 256 + 4 * LOG_FRAMES);
  // erpm_log1 = (uint16_t *)(logData + 256 + 6 * LOG_FRAMES);
  acceleration_log = (int16_t *)(logData + 256 + 4 * LOG_FRAMES);
  // bmi_temp_log = (int16_t *)(logData + 256 + 10 * LOG_FRAMES);
  // p_term_log0 = (uint16_t *)(logData + 256 + 12 * LOG_FRAMES);
  // p_term_log1 = (uint16_t *)(logData + 256 + 14 * LOG_FRAMES);
  // i_term_log0 = (uint16_t *)(logData + 256 + 16 * LOG_FRAMES);
  // i_term_log1 = (uint16_t *)(logData + 256 + 18 * LOG_FRAMES);
  // i2_term_log0 = (uint16_t *)(logData + 256 + 20 * LOG_FRAMES);
  // i2_term_log1 = (uint16_t *)(logData + 256 + 22 * LOG_FRAMES);
  // d_term_log0 = (uint16_t *)(logData + 256 + 24 * LOG_FRAMES);
  // d_term_log1 = (uint16_t *)(logData + 256 + 26 * LOG_FRAMES);
  // target_erpm_log = (uint32_t *)(logData + 256 + 28 * LOG_FRAMES);
  

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
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  #if TRANSMISSION_IND != 0
  pinMode(TRANSMISSION_PIN, OUTPUT);
  digitalWrite(TRANSMISSION_PIN, HIGH);
  #endif 

  ESCs[0] = new ESC(ESC1_OUTPUT_PIN, ESC1_INPUT_PIN, RMT_CHANNEL_0, RMT_CHANNEL_1, onESCError, onStatusChange);
  ESCs[1] = new ESC(ESC2_OUTPUT_PIN, ESC2_INPUT_PIN, RMT_CHANNEL_2, RMT_CHANNEL_3, onESCError, onStatusChange);

  delay(5);
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &escIR, true);
  timerAlarmWrite(timer, 1000000 / ESC_FREQ, true);
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

  //registering the light sensor
  lsStatePtr = initLightSensor(LIGHT_SENSOR_PIN, &onLightSensorChange);

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
  c1ready = true;
  while (!c0ready) {
    delay(1);
  }
  if (statusLED == LED_SETUP) resetStatusLED();
}