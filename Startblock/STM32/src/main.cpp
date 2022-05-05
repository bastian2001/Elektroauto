//BEFORE FLASHING: Make sure to cut the trace on PA12 to the resistor voltage divider. Solder a wire to PA8 instead from the voltage divider. The push button is NORMALLY CLOSED! Also, to prevent a voltage drop of 12V over the 100Ohm resistors with mono cables, move R8 and R10 to the blocks instead! Software will prevent this from happening for more than 150ms, but even 150ms can be bad for the resistor (and the IR-LEDs don't work anyway).




#include <Arduino.h>

#define BUTTON_PIN PB12
#define LED_PIN PB0
#define JACK_PIN PB1
#define LIN_PIN PA2
#define RIN_PIN PA1
// #define VIN_PIN PA7
// #define LOUT_ENDSTOP_PIN PB9
// #define ROUT_ENDSTOP_PIN PA8
#define LOUT_PIN PB7
#define ROUT_PIN PB6
// #define L_LED_VOLT_PIN PA4
// #define R_LED_VOLT_PIN PA5

#define CLOCK_CYCLES_PER_SECOND  72000000
#define MAX_RELOAD               0xFFFF
#define LED_FREQ 3000
#define LED_PERIOD_CYCLES CLOCK_CYCLES_PER_SECOND / LED_FREQ
#define LED_PRESCALER (uint16_t)(LED_PERIOD_CYCLES / MAX_RELOAD + 1)
#define LED_OVERFLOW (uint16_t)((LED_PERIOD_CYCLES + (LED_PRESCALER / 2)) / LED_PRESCALER) // Max PWM output

#define ADC_TO_VOLTAGE(x) (x * (3.3 * 69/18/4096))
#define VOLTAGE_TO_ADC(x) (x * ((float)4096*18/69/3.3))


HardwareTimer pwmTimer3(3);
// HardwareTimer pwmTimer4(4);

typedef struct output {
  uint8_t pin;
//   uint8_t endstopPin;
//   uint8_t voltagePin;
  uint8_t state;
  uint32_t lastChanged;
  uint32_t lastOnOffChange;
//   float ledVoltage;
//   bool connected;
//   bool ok;
  bool on;
} Output;

enum {
  STATE_OFF,
  STATE_IDLE,
  STATE_READY,
  STATE_START
};
enum {
  L = 0,
  R = 1
};

Output outputs[2];
Output& lOut = outputs[0];
Output& rOut = outputs[1];
float vin;
uint32_t autostartAt, blinkUntil, ledoffset;
// bool powerFromJack, vinOk, safety = true;
bool automatic, ledBlink;

void switchOn(Output& o, bool on){
  digitalWrite(o.pin, on /*&& vinOk*/);
  o.lastOnOffChange = millis();
}

void setState(Output& o, uint8_t state){
//   if (!o.ok && safety) state = STATE_OFF;
  o.state = state;
  if (state == STATE_START) switchOn(o, true);
  else switchOn(o, false);
  o.lastChanged = millis();
}

void lTrigger(){
  if (lOut.state == STATE_READY) setState(outputs[L], STATE_START);
  ledoffset = millis();
}
void rTrigger(){
  if (rOut.state == STATE_READY) setState(outputs[R], STATE_START);
  ledoffset = millis();
}

void setup() {
  Serial.begin(115200);
  adc_calibrate(&adc1);
  adc_calibrate(&adc2);

  rOut.pin = ROUT_PIN;
  lOut.pin = LOUT_PIN;
//   rOut.endstopPin = ROUT_ENDSTOP_PIN;
//   lOut.endstopPin = LOUT_ENDSTOP_PIN;
//   rOut.voltagePin = R_LED_VOLT_PIN;
//   lOut.voltagePin = L_LED_VOLT_PIN;

  pwmTimer3.pause();
  pwmTimer3.setPrescaleFactor(LED_PRESCALER);
  pwmTimer3.setOverflow(LED_OVERFLOW);
  pwmTimer3.refresh();
  pwmTimer3.resume();
//   pwmTimer4.pause();
  // pwmTimer4.setPrescaleFactor(BLOCK_PRESCALER);
  // pwmTimer4.setOverflow(BLOCK_OVERFLOW);
  // pwmTimer4.refresh();
  // pwmTimer4.resume();
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, PWM);
  pinMode(JACK_PIN, INPUT_PULLUP);
  pinMode(LIN_PIN, INPUT);
  pinMode(RIN_PIN, INPUT);
//   pinMode(VIN_PIN, INPUT);
//   pinMode(LOUT_ENDSTOP_PIN, INPUT);
//   pinMode(ROUT_ENDSTOP_PIN, INPUT);
  pinMode(LOUT_PIN, OUTPUT);
  pinMode(ROUT_PIN, OUTPUT);
//   pinMode(L_LED_VOLT_PIN, INPUT);
//   pinMode(R_LED_VOLT_PIN, INPUT);
  pwmWrite(LED_PIN, 0);
  setState(lOut, STATE_OFF);
  setState(rOut, STATE_OFF);

  attachInterrupt(LIN_PIN, &lTrigger, RISING);
  attachInterrupt(RIN_PIN, &rTrigger, RISING);

  delay(5000);
  Serial.println("vin\tvinOk\tbtnSt\tlCon\tlState\tlLEDV\tlOk\trCon\trState\trLEDV\trOk\tautom.\tstartAt");
  
  setState(outputs[0], STATE_IDLE);
  setState(outputs[1], STATE_IDLE);
  ledoffset = millis();
}

void disableStartPWM(){
  for (int i = 0; i < 2; i++){
    if (outputs[i].state == STATE_START && millis() > outputs[i].lastChanged + 1000){
      ledoffset = millis();
      setState(outputs[i], STATE_IDLE);
    }
  }
}

// uint32_t lastRVoltageError[2];
// uint32_t lastLEDVoltageError[2];
// void checkOutputs(){
//   for (int i = 0; i < 2; i++){
//     Output& o = outputs[i];
//     if (o.connected == digitalRead(o.endstopPin)){
//       uint32_t state = digitalRead(o.endstopPin);
//       uint32_t end = millis() + 20;
//       while (millis() < end){
//         if (digitalRead(o.endstopPin) != state){
//           end = millis() + 20;
//           state = !state;
//         } 
//       }
//       if (o.connected == state) o.connected = !o.connected;
//       else continue; //error in reading

//       if (o.connected) {
//         //freshly connected
//         Serial.print(i == R ? "Right" : "Left");
//         Serial.println(" connected");
//         o.ok = true;
//         setState(o, STATE_IDLE);
//       } else {
//         if (safety){
//           setState(o, STATE_OFF);
//         }
//         Serial.print(i == R ? "Right" : "Left");
//         Serial.println(" disconnected");
//         o.ok = false;
//         //freshly disconnected
//       }
//     }
//     o.ledVoltage = ADC_TO_VOLTAGE(analogRead(o.voltagePin));
//     if (millis() - o.lastOnOffChange > 10 && o.connected && millis() - o.lastChanged > 150){
//       if ((o.ledVoltage > 9.5 && o.on) || vin - o.ledVoltage > 6){
//         o.ok = false;
//         if (safety)
//           setState(o, STATE_OFF);
//       }
//     }
//   }
// }

// void checkPowerSupply(){
//   vin = ADC_TO_VOLTAGE(analogRead(VIN_PIN));
//   if (vin > 12.6 || (vin < 10 && safety)){
//     for (int i = 0; i < 10 && vinOk; i++){
//       if (ADC_TO_VOLTAGE(analogRead(VIN_PIN)) < 12.6 && ADC_TO_VOLTAGE(analogRead(VIN_PIN)) > 10) return;
//     }
//     if (!vinOk) return;
//     vinOk = false;
//     Serial.println("VIN out of bounds!");
//     setState(outputs[0], STATE_OFF);
//     setState(outputs[1], STATE_OFF);
//     blinkUntil = millis() + 1900;
//     ledBlink = true;
//   } else {
//     for (int i = 0; i < 10 && !vinOk; i++){
//       if (ADC_TO_VOLTAGE(analogRead(VIN_PIN)) > 12.6 || ADC_TO_VOLTAGE(analogRead(VIN_PIN)) < 10) return;
//     }
//     if (vinOk) return;
//     vinOk = true;
//     Serial.println("VIN ok again!");
//     setState(outputs[0], STATE_IDLE);
//     setState(outputs[1], STATE_IDLE);
//   }
//   powerFromJack = digitalRead(JACK_PIN);
// }

void doublePress(){
  setState(outputs[0], STATE_IDLE);
  setState(outputs[1], STATE_IDLE);
  automatic = !automatic;
  autostartAt = 0;
  for (int i = automatic ? 1 : 1000; automatic ? (i <= 1000) : (i > 0); i += automatic * 2 - 1){
    int j = i % 500;
    pwmWrite(LED_PIN, j * LED_OVERFLOW / 500);
    delay(1);
  }
  ledoffset = millis();
}

void shortPress(){
  ledoffset = millis();
  if (automatic && !autostartAt)
    autostartAt = millis() + 5000;
  else if (automatic && autostartAt)
    autostartAt = 0;
  if (outputs[0].state > STATE_IDLE || outputs[1].state > STATE_IDLE){
    setState(outputs[0], STATE_IDLE);
    setState(outputs[1], STATE_IDLE);
    autostartAt = 0;
  } else {
    if (!automatic){
      ledBlink = true;
      blinkUntil = millis() + 400 * 3+100;
      ledoffset -= 200;
    }
    setState(outputs[0], STATE_READY);
    setState(outputs[1], STATE_READY);
  }
}

void longPress(){
  autostartAt = 0;
//   safety = !safety;
  setState(outputs[0], STATE_IDLE);
  setState(outputs[1], STATE_IDLE);
  ledoffset = millis();
}

uint32_t lastButtonChange;
uint32_t lastButtonDown, lastButtonUp;
bool lastButtonState;
void buttonRoutine(){
  if (lastButtonChange + 20 > millis()) return;
  if (digitalRead(BUTTON_PIN) != lastButtonState){
    for (int i = 0; i < 20; i++){
      if (digitalRead(BUTTON_PIN) == lastButtonState)
      return;
      delay(1);
    }
    lastButtonState = !lastButtonState;
    lastButtonChange = millis();
    if (lastButtonState){
      lastButtonDown = lastButtonChange;
      //newly pressed
    } else {
      if (lastButtonChange - lastButtonUp < 1000){
        doublePress(); // switches mode - automatic, triggered
      } else if (lastButtonChange - lastButtonDown > 1000){
        longPress(); // switches safety errors off/on
      } else {
        shortPress(); // sets into ready mode
      }
      lastButtonUp = lastButtonChange;
      //newly released
    }
  }
}

void autostartRoutine(){
  if (autostartAt && autostartAt < millis()){
    rTrigger();
    lTrigger();
    autostartAt = 0;
  }
}

void btnLEDRoutine(){
  if (ledBlink){
    if (blinkUntil < millis()){
      ledBlink = false;
      ledoffset = millis();
      return;
    }
    uint32_t timePassed = millis() - ledoffset;
    timePassed %= 400;
    if (timePassed < 200){
      pwmWrite(LED_PIN, LED_OVERFLOW);
      return;
    }
    pwmWrite(LED_PIN, 0);
    return;
  }
  if (outputs[0].state > STATE_IDLE || outputs[1].state > STATE_IDLE){
    if (!automatic){
      if (millis() > blinkUntil)
        pwmWrite(LED_PIN, 0);
      else {
        int timeLeft = blinkUntil - millis();
        if (timeLeft < 0) timeLeft = 0;
        timeLeft %= 400;
        timeLeft += 200;
        timeLeft /= 400;
        pwmWrite (LED_PIN, timeLeft * LED_OVERFLOW);
      }
      return;
    }
    int timeLeft = autostartAt - millis();
    if (timeLeft > 2000){
      timeLeft %= 1000;
      timeLeft += 500;
      timeLeft /= 1000;
      pwmWrite(LED_PIN, LED_OVERFLOW * (1 - timeLeft));
    } else {
      timeLeft %= 500;
      timeLeft += 250;
      timeLeft /= 500;
      pwmWrite(LED_PIN, (1-timeLeft) * LED_OVERFLOW);
    }
    return;
  }
  if ((outputs[0].state == STATE_IDLE) && (outputs[1].state == STATE_IDLE)){
    int position = ((int)(millis() - ledoffset)) % 8000;
    position -= 4000;
    if (position < 0){
      pwmWrite(LED_PIN, 0);
      return;
    }
    if (position > 2000) position = 4000 - position;
    position /= 2;
    uint32_t x = ((uint32_t)(1.6*position))*position*position/(1000000+position*position) + .2 * position;
    position = LED_OVERFLOW * x / 1000;
    pwmWrite(LED_PIN, position);
    return;
  }
//   if (!(outputs[0].connected || outputs[1].connected)){
//     pwmWrite(LED_PIN, LED_OVERFLOW);
//     return;
//   }
//   if (outputs[R].connected && outputs[L].connected && !outputs[L].ok && !outputs[R].ok){
//     uint32_t timePassed = millis() - ledoffset;
//     timePassed %= 4500;
//     if (timePassed < 500 || (timePassed > 1000 && timePassed < 1500) || (timePassed > 2000 && timePassed < 2500)){
//       pwmWrite(LED_PIN, LED_OVERFLOW);
//       return;
//     }
//     pwmWrite(LED_PIN, 0);
//     return;
//   }
//   if (outputs[R].connected && !outputs[R].ok){
//     uint32_t timePassed = millis() - ledoffset;
//     timePassed %= 3500;
//     if (timePassed < 500 || (timePassed > 1000 && timePassed < 1500)){
//       pwmWrite(LED_PIN, LED_OVERFLOW);
//       return;
//     }
//     pwmWrite(LED_PIN, 0);
//     return;
//   }
//   if (outputs[L].connected && !outputs[L].ok){
//     uint32_t timePassed = millis() - ledoffset;
//     timePassed %= 2000;
//     if (timePassed < 500){
//       pwmWrite(LED_PIN, LED_OVERFLOW);
//       return;
//     }
//     pwmWrite(LED_PIN, 0);
//     return;
//   }
  uint32_t position = (millis() - ledoffset) % 4000;
  if (position > 2000) position = 4000 - position;
  position /= 2;
  uint32_t x = ((uint32_t)(1.6*position))*position*position/(1000000+position*position) + .2 * position;
  position = LED_OVERFLOW * x / 1000;
  pwmWrite(LED_PIN, position);
}

uint32_t lastSerialMillis;
uint32_t lastOutputChangeMillis;
bool cycle = false;
bool outputsOn = false;
// char sOut[1000];
void loop() {
  while ((millis() - lastOutputChangeMillis) >= 48){
    cycle = true;
    if ((millis() - lastOutputChangeMillis) >= 50) break;
  }
  if (cycle){
    outputsOn = !outputsOn;
    if (lOut.state == STATE_IDLE) digitalWrite(lOut.pin, outputsOn);
    if (rOut.state == STATE_IDLE) digitalWrite(rOut.pin, outputsOn);
    lastOutputChangeMillis = millis();
    cycle = false;
  }
  autostartRoutine();
  disableStartPWM();
//   checkPowerSupply(); //voltage and which source
//   checkOutputs(); //LED voltage and connected
  buttonRoutine();
  btnLEDRoutine();
//   if (millis() > lastSerialMillis + 2000){
//     lastSerialMillis = millis();
//     snprintf(sOut, 1000, "%4.2f\t%d\t%d\t%d\t%d\t%4.2f\t%d\t%d\t%d\t%4.2f\t%d\t%d\t%d", vin, vinOk, lastButtonState, outputs[0].connected, outputs[0].state, outputs[0].ledVoltage, outputs[0].ok, outputs[1].connected, outputs[1].state, outputs[1].ledVoltage, outputs[1].ok, automatic, (int)autostartAt);
//     Serial.println(sOut);
//   }
}