/*TODO:
 * (Durchschnitt der bisherigen Gaswerte)
 * anpassen, wobei das erst später geht
 * (Online-Interface)
 * Schlupf in Code implementieren
 */


/*======================================================includes======================================================*/

#include "WiFi.h"
#include "Wire.h"
#include "I2Cdev.h"
#include "MPU6050.h"
#include <WebSocketsServer.h>
#include "driver/rmt.h"



/*======================================================definitions======================================================*/

//Pin numbers
#define HS1 14
#define HS2 15
#define escOutputPin  16
#define escTriggerPin 33
#define TRANSMISSION  23

//PID loop settings
#define PID_DIV      5
#define trend_amount 9 //nur ungerade!!
#define ta_div_2     4 //mit anpassen!!

//DShot values
#define ESC_FREQ  1000
#define ESC_BUFFER_ITEMS 16
#define CLK_DIV 6; //DShot 150: 12, DShot 300: 6, DShot 600: 3
#define TT 44 // total bit time
#define T0H 17 // 0 bit high time
#define T1H 33 // 1 bit high time
#define T0L (TT - T0H) // 0 bit low time
#define T1L (TT - T1H) // 1 bit low time
#define T_RESET 21 // reset length in multiples of bit time
#define ESC_TELEMETRY_REQUEST false

//WiFi and WebSockets settings
#define ssid "KNS_WLAN"
#define password "YZKswQHaE4xyKqdP"
//#define ssid "Coworkingspace"
//#define password "suppenkasper"
#define maxWS 5
#define telemetryUpdateMS 50


/*======================================================global variables======================================================*/

//Core 0 variables
TaskHandle_t Task1;
String toBePrinted = "";

//rps control variables
int rps_target = 0;
int rps_was[trend_amount];
double rpm_a = .00000015;
double rpm_b = .000006;
double rpm_c = .006;

//hall sensor variables
int hs1c = 0, hs2c = 0, hsc = 0;

//MPU variables
MPU6050 mpu;
int16_t MPUoffset = 0, raw_accel = 0;
unsigned long lastMPUUpdate = 0;
int counterMPU = 0;
float acceleration = 0, speedMPU = 0, distMPU = 0;

//system variables
double throttle = 0;
bool armed = false, c1ready = false, irInUse = false;
int ctrlMode = 0, req_value = 0;
uint16_t escValue = 0;

//WiFi/WebSockets variables
WebSocketsServer webSocket = WebSocketsServer(80);
uint8_t clients[maxWS][2]; //[device (web, app, ajax, disconnected)][telemetry (off, on)]
unsigned long lastTelemetry = 0;
bool wifiFlag = false;

//ESC variables
rmt_item32_t esc_data_buffer[ESC_BUFFER_ITEMS];

//arbitrary variables
int16_t escOutputCounter = 0;


/*======================================================system methods======================================================*/

void setup() {

  //Serial setup
  Serial.begin(115200);

  //WiFi Setup
  WiFi.enableSTA(true);
  WiFi.setTxPower(WIFI_POWER_MINUS_1dBm);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to "); Serial.println(ssid);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println("WiFi-Connection could not be established!");
      Serial.println("Restart...");
      ESP.restart();
  }
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  //ESC pins Setup
  pinMode(escOutputPin, OUTPUT);
  pinMode(escTriggerPin, OUTPUT);
  pinMode(TRANSMISSION, OUTPUT);
  digitalWrite(TRANSMISSION, HIGH);
  esc_init(0, escOutputPin);
  ledcSetup(1, ESC_FREQ, 8);
  ledcAttachPin(escTriggerPin, 1);
  ledcWrite(1, 127);

  //rps_was Setup
  for (int i = 0; i < trend_amount; i++){
    rps_was[i] = 0;
  }
  
  //2nd core setup
  xTaskCreatePinnedToCore( Task1code, "Task1", 10000, NULL, 0, &Task1, 0);

  //WebSockets init
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);
  for (int i = 0; i<maxWS; i++){
    clients[i][0] = 3;
    clients[i][1] = 1;
  }

  //MPU6050 init
  /*Wire.begin(/*13,12*//*); //sda, scl
  mpu.initialize();
  Serial.println(mpu.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");
  int offsetCounter = 0;
  long offset_sum = 0;
  while(offsetCounter < 20){
    offsetCounter++;
    offset_sum += mpu.getAccelerationX();
    delay(1);
  }
  int MPUoffset = (int)((float)offset_sum / 20.0 + .5);
  mpu.setXAccelOffset(MPUoffset);*/

  //setup termination
  Serial.println("ready");
  c1ready = true;
  lastMPUUpdate = millis();
}

void Task1code( void * parameter) {
  pinMode(HS1, INPUT_PULLUP);
  pinMode(HS2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(HS1), hs1ir, CHANGE);
  attachInterrupt(digitalPinToInterrupt(HS2), hs2ir, CHANGE);
  attachInterrupt(digitalPinToInterrupt(escTriggerPin), escir, RISING);
  disableCore0WDT();
  
  while(!c1ready){yield();}
  
  while(true){
    loop0();
  }
}

void loop0(){
  if(WiFi.status()!=WL_CONNECTED){
    reconnect();
  }
  handleWiFi();
  receiveSerial();
  printSerial();
}

void loop() {
  if (lastMPUUpdate + 1 <= millis()){
    irInUse = true;
    raw_accel = mpu.getAccelerationY();
    irInUse = false;
    if(counterMPU++ % 20 == 0)
      sPrintln(String(raw_accel));
    counterMPU++;
    //MPU calculations
    acceleration = (float)raw_accel/32767*19.62;
    speedMPU += acceleration / 1000;
    distMPU += speedMPU / 1000;
    lastMPUUpdate = millis();
  }
}


/*======================================================functional methods======================================================*/

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length){
  switch(type) {
    case WStype_DISCONNECTED:
      removeClient(num);
      break;
    case WStype_CONNECTED:
      addClient(num);
      break;
    case WStype_TEXT:
      dealWithMessage((char*)payload, num);
      break;
    default:
      break;
  }
}

void removeClient (int spot){
  Serial.printf("[%u] Disconnected!\n", spot);
  clients[spot][0] = 3;
  clients[spot][1] = 1;
}

void addClient (int spot){
  if(spot < maxWS){
    IPAddress ip = webSocket.remoteIP(spot);
    Serial.printf("[%u] Connection from ", spot);
    Serial.println(ip.toString());
  } else {
    Serial.println(F("Connection refused. All spots filled!"));
  }
}

void dealWithMessage(String message, uint8_t from){
  Serial.print(F("Received: \""));
  Serial.print(message);
  Serial.println(F("\""));
  switch (message.charAt(0)){
    case 's': //setup message
      Serial.println(F("Parsing setup message"));
      parseSystemMessage(message.substring(1), from);
      break;
    case 'c': //control message
      Serial.println(F("Parsing control message"));
      parseControlMessage(message.substring(1));
      break;
    default:
      Serial.println(F("Unknown message format!"));
      break;
  }
}

void reconnect(){
  detachInterrupt(digitalPinToInterrupt(escTriggerPin));
  detachInterrupt(digitalPinToInterrupt(HS1));
  detachInterrupt(digitalPinToInterrupt(HS2));
  WiFi.disconnect();
  int counterWiFi = 0;
  while(WiFi.status()==WL_CONNECTED){yield();}
  Serial.print("Reconnecting");
  WiFi.begin(ssid, password);
  while(WiFi.status()!=WL_CONNECTED){
    delay(300);
    Serial.print(".");
    if(counterWiFi == 30){
      Serial.println();
      Serial.println("WiFi-Connection could not be established!");
      break;
    }
    counterWiFi++;
  }
  Serial.println();
  hs2c = 0;
  hs1c = 0;
  hsc = 0;
  attachInterrupt(digitalPinToInterrupt(escTriggerPin), escir, RISING);
  attachInterrupt(digitalPinToInterrupt(HS1), hs1ir, CHANGE);
  attachInterrupt(digitalPinToInterrupt(HS2), hs2ir, CHANGE);
}

void receiveSerial(){
  if(Serial.available()){
    String readout = Serial.readStringUntil('\n');
    int val = readout.substring(1).toInt();
    switch(readout.charAt(0)){
      case 'a':
        armed = val > 0;
        ctrlMode = 0;
        setThrottle(0);
        break;
      case 'd':
        ctrlMode = 0;
        setThrottle(val);
        break;
      case 'r':
        ctrlMode = 1;
        rps_target = val;
        break;
      case 'p':
        proportional = val;
        break;
      case 'c':
        reconnect();
        break;
      default:
        Serial.println("Not found!");
        break;
    }
  }
}

void escir(){
  int counts = hsc;
  hs2c = 0;
  hs1c = 0;
  hsc = 0;

  for (int i = 0; i<trend_amount-1; i++){
    rps_was[i] = rps_was[i+1];
  }
  rps_was[trend_amount-1] = counts * ESC_FREQ / 12; //6 für einen Sensor, 12 für 2
  if (armed) {

    if (wifiFlag){
      wifiFlag = false;
      switch (ctrlMode){
        case 0:
          setThrottle(req_value);
          break;
        case 1:
          rps_target = req_value;
          break;
        case 2:
          break;
        default:
          break;
      }
    }
    switch (ctrlMode){
      case 0:
        break;
      case 1:
        setThrottle(calcThrottle(rps_target, rps_was) + .5);
        break;
      case 2:
        break;
      default:
        break;
    }
  } else {
    setThrottle(0);
  }
  digitalWrite(TRANSMISSION, LOW);
  esc_send_value(escValue, false);
  digitalWrite(TRANSMISSION, HIGH);
}

void setThrottle(int newThrottle){ //throttle value between 0 and 2000 --> esc value between 0 and 2047 with checksum
  throttle = newThrottle;
  newThrottle += 47;
  if (newThrottle == 47)
    newThrottle = 0;
  escValue = appendChecksum(newThrottle);
}

double calcThrottle(int target, int was[]){
  double was_avg = 0;
  int was_sum = 0, t_sq_sum = 0, t_multi_was_sum = 0;
  for(int i = 0; i < trend_amount; i++){
    int t = i - ta_div_2;
    was_sum += was[i];
    t_sq_sum += pow(t, 2);
    t_multi_was_sum += t * was[i];
  }
  was_avg = (double)was_sum / trend_amount;

  double m = (double)t_multi_was_sum / (double)t_sq_sum;
  
  double prediction = m * ((double)trend_amount - (double)ta_div_2) + (double)was_avg;
  double delta_rpm = target - prediction;
  double delta_throttle = rpm_a * pow(delta_throttle, 3) + rpm_b * pow(delta_throttle, 2) + rpm_c * delta_throttle;

  throttle += delta_throttle;
  
  return throttle;
}

void handleWiFi(){
  while(irInUse){yield();}
  irInUse = true;
  webSocket.loop();
  irInUse = false;
  if (lastTelemetry + telemetryUpdateMS < millis()){
    lastTelemetry = millis();
    sendTelemetry();
  }
}

void parseSystemMessage(String clippedMessage, uint8_t id){
  while (clippedMessage.indexOf("!") == 0){
    clippedMessage = clippedMessage.substring(clippedMessage.indexOf("!")+1);

    //parse single command
    switch (clippedMessage.charAt(0)){
      case 'd':
        clients[id][0] = clippedMessage.substring(1).toInt();
        break;
      case 't':
        clients[id][1] = clippedMessage.substring(1).toInt();
        break;
      default:
        Serial.println("unknown value given (setup)");
        break;
    }
    
    clippedMessage = clippedMessage.substring(clippedMessage.indexOf("!"));
  }
}

void parseControlMessage(String clippedMessage){
  while (clippedMessage.indexOf("!") == 0){
    clippedMessage = clippedMessage.substring(clippedMessage.indexOf("!")+1);

    //parse single command
    switch (clippedMessage.charAt(0)){
      case 'a':
        armed = clippedMessage.substring(1).toInt() > 0;
        setThrottle(0);
        break;
      case 'm':
        ctrlMode = clippedMessage.substring(1).toInt();
        break;
      case 'v':
        req_value = clippedMessage.substring(1).toInt();
        break;
      default:
        Serial.println("unknown value given (control)");
    }
    
    clippedMessage = clippedMessage.substring(clippedMessage.indexOf("!"));
  }
  wifiFlag = true;
}

void sendTelemetry(){
  int velMPU = (int)(speedMPU * 1000 + .5);
  int velWheel = (int)((float)30 * PI * (float)rps_was[trend_amount-1]);
  int slipPercent = 0;
  if(velWheel != 0){
    slipPercent = (float)(velWheel - velMPU) / velWheel * 100;
  }
  String telemetryData0 = "";
  String telemetryData1 = "a";
  String telemetryData2 = "";
  telemetryData1 += armed ? "1" : "0";
  telemetryData1 += "!m";
  telemetryData1 += ctrlMode;
  telemetryData1 += "!t";
  telemetryData1 += ((int) throttle);
  telemetryData1 += "!r";
  telemetryData1 += rps_was[trend_amount - 1];
  telemetryData1 += "!s";
  telemetryData1 += slipPercent;
  telemetryData1 += "!v";
  telemetryData1 += velMPU;
  telemetryData1 += "!w";
  telemetryData1 += velWheel;
  telemetryData1 += "!c";
  telemetryData1 += ((int)(acceleration * 1000 + .5));
  for (int i = 0; i < maxWS; i++){
    if (clients[i][0] == 1 && clients[i][1] == 1){
      while(irInUse){yield();}
      irInUse = true;
      webSocket.sendTXT(i, telemetryData1);
      irInUse = false;
    }
  }
}

void hs1ir(){
  hs1c++;
  hsc++;
}

void hs2ir(){
  hs2c++;
  hsc++;
}

void printSerial(){
  Serial.print(toBePrinted);
  toBePrinted = "";
}

void sPrint(String s){
  toBePrinted += s;
}

void sPrintln(String s){
  toBePrinted += s;
  toBePrinted += "\n";
}

uint16_t appendChecksum(uint16_t value){
  value &= 0x7FF;
  value = (value << 1) | ESC_TELEMETRY_REQUEST;
  int csum = 0, csum_data = value;
  for (int i = 0; i < 3; i++) {
    csum ^=  csum_data;   // xor data by nibbles
    csum_data >>= 4;
  }
  csum &= 0xf;
  value = (value << 4) | csum;
  return value;
}

void esc_init(uint8_t channel, uint8_t pin){
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

void esc_send_value(uint16_t value, bool wait) {
  setup_rmt_data_buffer(value);
  ESP_ERROR_CHECK(rmt_write_items((rmt_channel_t) 0, esc_data_buffer, ESC_BUFFER_ITEMS, wait));
}

void setup_rmt_data_buffer(uint16_t value) {
    uint16_t mask = 1 << (ESC_BUFFER_ITEMS - 1);
    for (uint8_t bit = 0; bit < ESC_BUFFER_ITEMS; bit++) {
      uint16_t bit_is_set = value & mask;
      esc_data_buffer[bit] = bit_is_set ? (rmt_item32_t){{{T1H, 1, T1L, 0}}} : (rmt_item32_t){{{T0H, 1, T0L, 0}}};
      mask >>= 1;
    }
}
