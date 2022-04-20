//receiver

//13 - data - D7
//5 - transmission indicator - D1


#define NO_OF_MEASUREMENTS 500
#define LED_BUILTIN 22
#define SLOWNESS 0  //Dshot: 1 or so, PWM 320

bool result[NO_OF_MEASUREMENTS];
unsigned long mic = 0, lastMics = 0;
uint8_t counter = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin (230400);
  Serial.println("almost ready");
  pinMode(13, INPUT);
  pinMode(5, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(200);
  digitalWrite(LED_BUILTIN, LOW);
  delay(200);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(200);
  digitalWrite(LED_BUILTIN, LOW);
  delay(200);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(200);
  digitalWrite(LED_BUILTIN, LOW);
  delay(200);
  Serial.println("ready");
}

void loop() {
  // put your main code here, to run repeatedly:
  if(!digitalRead(5)){
    //mic = micros();
    for (int i = 0; i < NO_OF_MEASUREMENTS; i++){
      result[i] = digitalRead(13);
      #if SLOWNESS > 0
        for (int i = 0; i < SLOWNESS; i++){
          digitalRead(1);
        }
      #endif
    }
    //mic = micros() - mic;
    for (int i = 0; i < NO_OF_MEASUREMENTS; i++){
      Serial.println(result[i]);
    }
    //Serial.println(mic);
    while (!digitalRead(5));
  delay(1000);
  }
}
