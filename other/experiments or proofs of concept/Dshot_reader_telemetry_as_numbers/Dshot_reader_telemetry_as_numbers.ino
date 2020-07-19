#define AMOUNT 10

char chars[AMOUNT];
//int counter = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("ready");
  delay(10);
  Serial.swap();
}

void loop() {
  // put your main code here, to run repeatedly:
  /*if (Serial.available()){
    delay(5);
    int* numbers = 0;
    numbers = new int[Serial.available()];
    for (int i = 0; i < sizeof(numbers) / sizeof(int); i++){
      numbers[i] = Serial.read();
    }
    Serial.swap();
    for (int i = 0; i < sizeof(numbers) / sizeof(int); i++){
      Serial.print(numbers[i], BIN);
      Serial.println();
    }
    Serial.swap();
  }*/
  /*if (Serial.available() && counter < AMOUNT){
    chars[counter] = (char)Serial.read();
    counter++;
  }*/
  if (Serial.available()){
    for (int i = 0; i < AMOUNT - 1; i++){
      chars[i] = chars[i+1];
    }
    chars[AMOUNT - 1] = (char)Serial.read();
  }
  if (isComplete()){
    Serial.swap();
    Serial.print(chars[0], DEC);
    Serial.print("\t");
    Serial.print((float)((int)((chars[1] << 8) | chars[2])) / 100.0);
    Serial.print("\t");
    Serial.print((float)((int)((chars[7] << 8) | chars[8])) * 100.0 / 420.0);
    Serial.println();
    for (int i = 0; i < AMOUNT; i++){
      chars[i] = B1;
    }
    Serial.flush();
    Serial.swap();
  }
}

bool isComplete(){
  return chars[3] == 0 && chars[4] == 0 && chars[5] == 0 && chars[6] == 0;
}
