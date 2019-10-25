/*TODO:
 * (Durchschnitt der bisherigen Gaswerte)
 * anpassen, wobei das erst später geht
 * (Online-Interface)
 * Schlupf in Code implementieren
 */

#include "WiFi.h"
//#include "Wire.h"
#include "I2Cdev.h"
#include "MPU6050.h"
#include <WebSocketsServer.h>

#define HS1 14
#define HS2 15
#define escPin  16
#define freq   200
#define trend_amount 9 //nur ungerade!!
#define ta_div_2     4 //mit anpassen!!

#define ssid "KNS_WLAN"
#define password "YZKswQHaE4xyKqdP"

#define maxWS 5

TaskHandle_t Task1;
String toBePrinted = "";

int rps_target = 0;
int rps_was[trend_amount];

int hs1c = 0, hs2c = 0, hsc = 0;

int MPUoffset = 0, raw_accel = 0;
unsigned long lastMPUUpdate = 0;
int counterMPU = 0;
float acceleration = 0, speedMPU = 0;

bool armed = false, wifiFlag = false, c1ready = false;
int ctrlMode = 0, req_value = 0;
double currentDur = 1000;

double proportional = 40.0;

WebSocketsServer webSocket = WebSocketsServer(80);
uint8_t clients[maxWS][2]; //[num][device]
unsigned long lastTelemetry = 0;
MPU6050 mpu;

void setup() {
  Serial.begin(115200);

  //WiFi Setup
  WiFi.enableSTA(true);
  WiFi.setTxPower(WIFI_POWER_MINUS_1dBm);
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  int counterWiFi = 0;
  while (WiFi.status() != WL_CONNECTED){  //"Connecting..."
    delay(300);
    Serial.print(".");
    if(counterWiFi == 30){
      Serial.println();
      Serial.println("WiFi-Connection could not be established!");
      Serial.println("Restart...");
      ESP.restart();
    }
    counterWiFi++;
  }
  Serial.println();
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  //escPin Setup
  pinMode(escPin, OUTPUT);
  ledcAttachPin(escPin, 1);
  ledcSetup(1, freq, 15);
  ledcWrite(1, map(1000, 0, 1000000/freq, 0, 32767));

  //Setup rps_was
  for (int i = 0; i < trend_amount; i++){
    rps_was[i] = 0;
  }
  
  //2nd core
  xTaskCreatePinnedToCore( Task1code, "Task1", 10000, NULL, 0, &Task1, 0);

  //MPU6050 init
  Wire.begin(2,12); //sda, scl
  int offsetCounter = 0;
  long offset_sum = 0;
  while(offsetCounter < 20){
    offsetCounter++;
    offset_sum += mpu.getAccelerationX();
    delay(1);
  }
  int MPUoffset = (int)((float)offset_sum / 20.0 + .5);
  mpu.setXAccelOffset(MPUoffset);

  //WS init
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);
  
  Serial.println("ready");
  c1ready = true;
  lastMPUUpdate = millis();
}

void loop() {
  if (lastMPUUpdate + 1 <= millis()){
    raw_accel = mpu.getAccelerationX();
    counterMPU++;
    if(counterMPU % 10)
      sPrintln(String(raw_accel));

    //MPU calculations
    acceleration = (float)raw_accel/32767*19.62;
    speedMPU += acceleration / 1000;
  }
}

void loop2(){
  if(WiFi.status()!=WL_CONNECTED){
    reconnect();
  }
  receiveWiFi();
  receiveSerial();
  printSerial();
}

void printSerial(){
  Serial.println(toBePrinted);
  toBePrinted = "";
}

void sPrint(String s){
  toBePrinted += s;
}

void sPrintln(String s){
  toBePrinted += s;
  toBePrinted += "\n";
}

void Task1code( void * parameter) {
  pinMode(HS1, INPUT_PULLUP);
  pinMode(HS2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(HS1), hs1ir, CHANGE);
  attachInterrupt(digitalPinToInterrupt(HS2), hs2ir, CHANGE);
  attachInterrupt(digitalPinToInterrupt(escPin), escir, RISING);
  while(!c1ready){delay(1);}
  while(true){
    loop2();
    delay(1);
  }
}

void reconnect(){
  detachInterrupt(digitalPinToInterrupt(escPin));
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
  attachInterrupt(digitalPinToInterrupt(escPin), escir, RISING);
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
        currentDur = 1000;
        break;
      case 'd':
        ctrlMode = 0;
        currentDur = val;
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

void hs1ir(){
  hs1c++;
  hsc++;
}

void hs2ir(){
  hs2c++;
  hsc++;
}

void escir(){
  if (armed) {
    int counts = hsc;
    hs2c = 0;
    hs1c = 0;
    hsc = 0;

    for (int i = 0; i<trend_amount-1; i++){
      rps_was[i] = rps_was[i+1];
    }
    rps_was[trend_amount-1] = counts * freq / 12; //6 für einen Sensor, 12 für 2

    if (wifiFlag){
      wifiFlag = false;
      switch (ctrlMode){
        case 0:
          currentDur = req_value + 1000;
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
        newDur((int)currentDur + .5);
        break;
      case 1:
        newDur((int)(calcDauer(rps_target, rps_was) + .5));
        break;
      case 2:
        break;
      default:
        break;
    }
  } else {
    currentDur = 1000.0;
    newDur(1000);
  }
}

void newDur(int dur){
  dur = max(dur, 1000);
  dur = min(dur, 1050);
  if(dur==1000)
    dur--;
  dur = map(dur, 0, 1000000/freq, 0, 32767);
  ledcWrite(1, dur);
}

double calcDauer(int target, int was[]){
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
  double delta_dur = ((double)target - prediction) / proportional;

  double newDur = currentDur + delta_dur;
  newDur = min(newDur, 1050.0);
  newDur = max(newDur, 1000.0);
  currentDur = newDur;
  
  return newDur;
}

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length){
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      for (uint8_t i = 0; i < maxWS; i++){
        if (clients[i][0]==num){
          clients[i][0]=NULL;
          clients[i][1]=NULL;
          clients[i][2]=NULL;
        }
      }
      break;
 
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connection from ", num);
        Serial.println(ip.toString());
        uint8_t spotFree = -1;
        for (int i = maxWS - 1; i >=0; i++){
          if(clients[i][0] == NULL){
            spotFree = i;
          }
        }
        if (spotFree > -1){
          Serial.print("Assigning spot "); Serial.println (spotFree);
          clients[spotFree][0] = num;
          clients[spotFree][2] = 1;
        } else {
          Serial.println("Connection refused. All spots filled!");
        }
      }
      break;
 
    case WStype_TEXT:
      {
        String message = (char*)payload;
        switch (message.charAt(0)){
          case 's': //setup message
            {
              uint8_t from = -1;
              for (int i = maxWS - 1; i >=0; i++){
                if(clients[i][0] == NULL){
                  from = i;
                }
              }
              parseSetupMessage(message.substring(1), from);
            }
            break;
          case 'c': //control message
            parseControlMessage(message.substring(1));
            break;
          default:
            Serial.println("Unknown message format");
            break;
        }
      }
 
    // For everything else: do nothing
    case WStype_BIN:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    default:
      break;
  }
}

void receiveWiFi(){
  webSocket.loop();
  if (lastTelemetry + 100 < millis()){
    lastTelemetry = millis();
    sendTelemetry();
  }
}

void parseSetupMessage(String clippedMessage, uint8_t id){
  while (clippedMessage.indexOf("\n") == 0){
    clippedMessage = clippedMessage.substring(clippedMessage.indexOf("\n")+1);

    //parse single command
    switch (clippedMessage.charAt(0)){
      case 'd':
        clients[id][1] = clippedMessage.substring(1).toInt();
        break;
      case 't':
        clients[id][2] = clippedMessage.substring(1).toInt();
        break;
      default:
        Serial.println("unknown value given");
    }
    
    clippedMessage = clippedMessage.substring(clippedMessage.indexOf("\n"));
  }
}

void parseControlMessage(String clippedMessage){
  while (clippedMessage.indexOf("\n") == 0){
    clippedMessage = clippedMessage.substring(clippedMessage.indexOf("\n")+1);

    //parse single command
    switch (clippedMessage.charAt(0)){
      case 'a':
        armed = clippedMessage.substring(1).toInt() > 0;
        currentDur = 1000;
        break;
      case 'm':
        ctrlMode = clippedMessage.substring(1).toInt();
        break;
      case 'v':
        req_value = clippedMessage.substring(1).toInt();
        break;
      default:
        Serial.println("unknown value given");
    }
    
    clippedMessage = clippedMessage.substring(clippedMessage.indexOf("\n"));
  }
}

void sendTelemetry(){
  int velMPU = (int)(speedMPU * 1000 + .5);
  int velWheel = (int)((float)30 * PI * (float)rps_was[trend_amount-1]);
  int slipPercent = (velWheel - velMPU) / velWheel;
  String telemetryData = "a";
  telemetryData += armed ? "1" : "0";
  telemetryData += "\nm";
  telemetryData += ctrlMode;
  telemetryData += "\nt";
  telemetryData += ((int)currentDur - 1000);
  telemetryData += "\nr";
  telemetryData += rps_was[trend_amount - 1];
  telemetryData += "\ns";
  telemetryData += slipPercent;
  telemetryData += "\nv";
  telemetryData += velMPU;
  telemetryData += "\nw";
  telemetryData += velWheel;
  telemetryData += "\nc";
  telemetryData += ((int)(acceleration * 1000 + .5));
  for (int i = 0; i < maxWS; i++){
    if (clients[i][0] != NULL && clients[i][2] == 1){
      webSocket.sendTXT(i, telemetryData);
    }
  }
}
