int duration = 1000;

void setup() {
  Serial.begin(115200);
  // put your setup code here, to run once:

  pinMode(5, OUTPUT);

  ledcAttachPin(5, 0);
  ledcSetup(0, 50, 15);
  ledcWrite(0, convertDuration(1000));
  Serial.println("Writing 1000 or 1638 to the ESC..");
}

int convertDuration(int dur){
  return map(dur, 0, 20000, 0, 32768);
}

void loop() {
  // put your main code here, to run repeatedly:
  if(Serial.available()>0){
    String readout = Serial.readStringUntil('\n');
    ledcWrite(0, convertDuration(readout.toInt()));
    Serial.print("Writing ");
    Serial.print(readout);
    Serial.print(" or ");
    Serial.print(convertDuration(readout.toInt()));
    Serial.println(" to the ESC...");
  }
}
