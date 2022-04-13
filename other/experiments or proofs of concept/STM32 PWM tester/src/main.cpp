//BEFORE FLASHING: Make sure to cut the trace on PA12 to the resistor voltage divider. Solder a wire to PA8 instead from the voltage divider. The push button is NORMALLY CLOSED! Also, to prevent a voltage drop of 12V over the 100Ohm resistors with mono cables, move R8 and R10 to the blocks instead! Software will prevent this from happening for more than 100ms, but even 100ms can be bad for the resistor (and the IR-LEDs don't work anyway).




#include <Arduino.h>

#define LOUT_PIN PB7

#define CLOCK_CYCLES_PER_SECOND  72000000
#define MAX_RELOAD               0xFFFF
#define BLOCK_FREQ 1200
#define BLOCK_PERIOD_CYCLES CLOCK_CYCLES_PER_SECOND / BLOCK_FREQ
#define BLOCK_PRESCALER (uint16_t)(BLOCK_PERIOD_CYCLES / MAX_RELOAD + 1)
#define BLOCK_OVERFLOW (uint16_t)((BLOCK_PERIOD_CYCLES + (BLOCK_PRESCALER / 2)) / BLOCK_PRESCALER) // Max PWM output

#define IDLE_PWM BLOCK_OVERFLOW / 4
#define MULTIPLIER_IDLE 0.7568 //8.4*4/(3*12+1*8.4)
#define READY_PWM BLOCK_OVERFLOW / 2
#define MULTIPLIER_READY 0.8236
#define START_PWM BLOCK_OVERFLOW * 3 / 4
#define MULTIPLIER_START 0.9032


HardwareTimer pwmTimer4(4);

void setup() {
  Serial.begin(115200);
  adc_calibrate(&adc1);
  adc_calibrate(&adc2);

  pwmTimer4.pause();
  pwmTimer4.setPrescaleFactor(BLOCK_PRESCALER);
  pwmTimer4.setOverflow(BLOCK_OVERFLOW);
  pwmTimer4.refresh();
  pwmTimer4.resume();

  pinMode(LOUT_PIN, PWM);
#define BUTTON_PIN PB12
  pinMode(BUTTON_PIN, INPUT_PULLUP);
#define LED_PIN PB0
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

      pwmWrite(LOUT_PIN, 0);
}

int readBtn(){
  uint32_t endMillis = millis() + 20;
  while (millis() < endMillis){
    if (!digitalRead(BUTTON_PIN)) return 0;
  }
  return 1;
}

void loop() {
  if (readBtn()){

  digitalWrite(LED_PIN, HIGH);
    delay(1000);
      pwmWrite(LOUT_PIN, IDLE_PWM);
    delay(5);
      pwmWrite(LOUT_PIN, READY_PWM);
    delay(5);
      pwmWrite(LOUT_PIN, START_PWM);
    delay(15);
      pwmWrite(LOUT_PIN, 0);
  digitalWrite(LED_PIN, LOW);
  }
}