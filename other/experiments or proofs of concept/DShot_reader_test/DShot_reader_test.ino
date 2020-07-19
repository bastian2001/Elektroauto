//receiver

//13 - data
//5 - transmission indicator


#define NO_OF_MEASUREMENTS 500
#define EMPTY_SPACE 500
#define SLOWNESS 3

bool result[NO_OF_MEASUREMENTS];
unsigned long mic = 0, lastMics = 0;
uint8_t numbers[] = {0,0,0,0,0,0,0,0,0,0};
uint8_t counter = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin (115200);
  Serial.println("almost ready");
  pinMode(13, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  Serial.println("ready");
}

void loop() {
  // put your main code here, to run repeatedly:
  if(Serial.available()){
    Serial.read();
    mic = micros();
    for (int i = 0; i < NO_OF_MEASUREMENTS; i++){
      result[i] = digitalRead(13);
      for (int i = 0; i < SLOWNESS; i++){
        digitalRead(13);
      }
    }
    mic = micros() - mic;
    /*for (int i = 0; i < EMPTY_SPACE; i++){
      Serial.println(1);
    }*/
    for (int i = 0; i < NO_OF_MEASUREMENTS; i++){
      Serial.println(result[i]);
    }
    //Serial.println(mic);
    while(digitalRead(5)){yield();}
  }
}
