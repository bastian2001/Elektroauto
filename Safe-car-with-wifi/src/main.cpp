/// @file main.cpp the entry point for the project


/*====================================================includes====================================================*/

#include "global.h"
#include "system.h"
#include "escIR.h"
#include "WiFiHandler.h"
#include "settings.h"
#include "LED.h"
#include "button.h"
#include "lightSensor.h"
#include "ESCPassthrough.h"
#include "race.h"

/*====================================================global variables================================================*/
extern bool lightSensorStart;
//Core 0 variables
TaskHandle_t core0Task;
bool c1ready = false, c0ready = false;

hw_timer_t * timer = NULL;
uint32_t lastSend = 0;
uint32_t lastCore0, lastCore1;





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
  if(WiFi.status() != WL_CONNECTED && !raceStopAt){
    pauseLS();
    timerAlarmDisable(timer);
    reconnect();
    timerAlarmEnable(timer);
    resumeLS();
  }

  handleWiFi();
  receiveSerial();

  checkButton();
  ledRoutine();

  printSerial();

  if (commitFlag){
    timerAlarmDisable(timer);
    pauseLS();
    commitFlag = false;
    EEPROM.commit();
    // if (escPassthroughMode) ESP.restart();
    timerAlarmEnable(timer);
    resumeLS();
  }
  lastCore0 = millis();
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
  setRaceThrottle();
  if (raceStopAt && raceStopAt < millis()){
    enableRaceMode(false);
  }
  if (lightSensorStart){
    lightSensorStart = false;
    startRace();
  }
  ESCs[0]->loop();
  ESCs[1]->loop();
  if (lastSend < millis() && !timerAlarmEnabled(timer)) sendESC = true;
  if (sendESC){
    sendESC = false;
    ESCs[0]->send();
    ESCs[1]->send();
    lastSend=millis();
  }
  if(lastCore0 + 5000 < millis()){
    Serial.println("Core 0 not responding");
    delay(2);
  }
  if (lastCore0 + 15000 < millis()){
    Serial.println("Core 0 didn't respond, restart...");
    delay(5);
    ESP.restart();
  }
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
  pinMode(LED_PIN, OUTPUT);
  setStatusLED(LED_SETUP);
  ledRoutine();
  //Serial setup

  //reading settings from EEPROM or writing them
  EEPROM.begin(77);
  if (firstStartup())
    writeEEPROM();
  else
    readEEPROM();

  if (escPassthroughMode){
    disableCore1WDT();
    EEPROM.writeUChar(76,0);
    EEPROM.commit();
    Serial.begin(115200);
  }
  while(escPassthroughMode){
    processPassthrough(escPassthroughMode);
  }

  Serial.begin(500000);

  delay(100);
  Serial.println();
  

  //WiFi Setup
  WiFi.enableSTA(true);
  WiFi.setTxPower(WIFI_POWER_13dBm);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  #ifdef PRINT_SETUP
    Serial.print("Connecting to "); Serial.println(ssid);
  #endif
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    #ifdef PRINT_SETUP
      Serial.println("WiFi error");
      // Serial.println("Restart...");
    #endif
    // ESP.restart();
  } else {
  #ifdef PRINT_SETUP
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  #endif
  }
  delay(20);

  //ESC pins Setup
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  ESCs[0] = new ESC(ESC1_OUTPUT_PIN, 21, RMT_CHANNEL_0, RMT_CHANNEL_2);
  ESCs[1] = new ESC(ESC2_OUTPUT_PIN, 19, RMT_CHANNEL_1, RMT_CHANNEL_3);

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