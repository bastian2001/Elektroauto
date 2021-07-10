/// @file main.cpp the entry point for the project


/*======================================================includes======================================================*/

#include "global.h"
#include "system.h"
#include "wifiStuff.h"
#include "escFunctions.h"
#include "settings.h"
#include "telemetry.h"
#include "accelerometerFunctions.h"


/*======================================================global variables==================================================*/

//Core 0 variables
TaskHandle_t core0Task;
bool c1ready = false;

hw_timer_t * timer = NULL;





/*======================================================system methods====================================================*/

/**
 * @brief loop on core 0
 * 
 * the not so time-sensitive stuff
 * initiates:
 * - wifi reconnection if neccessary
 * - race mode values sending
 * - low voltage checking
 * - wifi and serial message handling
 * - Serial string printing
 */
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

/**
 * @brief loop on core 1
 * 
 * time sensitive stuff
 * initiates:
 * - telemetry acquisition and processing
 * - throttle routine
 */
void loop() {
  getTelemetry();
  throttleRoutine();
  if (commitFlag){
    timerAlarmDisable(timer);
    commitFlag = false;
    EEPROM.commit();
    timerAlarmEnable(timer);
  }
}

//! @brief Task for core 0, creates loop0
void core0Code( void * parameter) {
  Serial1.begin(115200, SERIAL_8N1, ESC1_INPUT_PIN);
  Serial2.begin(115200);

  while (!c1ready) {
    yield();
  }

  while (true) {
    loop0();
  }
}

/**
 * @brief setup function
 * 
 * enables Serial communication
 * connects to wifi
 * sets up pins and timer for escir
 * creates task on core 0
 * initates websocket server
 */
void setup() {
  //reading settings from EEPROM or writing them
  EEPROM.begin(44);
  if (firstStartup())
    writeEEPROM();
  else
    readEEPROM();

  //logData initialization
  logData = (uint16_t *)malloc(LOG_FRAMES * 11);
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

  //BMI initialization
  initBMI();


  //ESC pins Setup
  pinMode(ESC1_OUTPUT_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  #if TRANSMISSION_IND != 0
  pinMode(TRANSMISSION_PIN, OUTPUT);
  digitalWrite(TRANSMISSION_PIN, HIGH);
  #endif
  esc_init(RMT_CHANNEL_0, ESC1_OUTPUT_PIN);
  esc_init(RMT_CHANNEL_1, ESC2_OUTPUT_PIN);
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
}