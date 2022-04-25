#include "global.h"
#include "LED.h"

bool lastLEDState = LOW;
bool newState;

bool setStatusLED(int newStatus){
  if (newStatus <= statusLED) return false;
  statusLED = newStatus;
  return true;
}
void resetStatusLED(){
  statusLED = 0;
}

void ledRoutine(){
  switch (statusLED)
  {
  case LED_OFF:
    if (lastLEDState) digitalWrite(LED_PIN, lastLEDState = LOW);
    break;
  case LED_SETUP:
    if (!lastLEDState) digitalWrite(LED_PIN, lastLEDState = HIGH);
    break;
  case LED_NO_DEVICE:
    newState = millis() % 1000 < 850;
    if (lastLEDState != newState) digitalWrite(LED_PIN, lastLEDState = newState);
    break;
  case LED_RACE_MODE:
    newState = millis() % 1000 < 500;
    if (lastLEDState != newState) digitalWrite(LED_PIN, lastLEDState = newState);
    break;
  case LED_FOUND_BLOCK:
    newState = millis() % 1000 < 150;
    if (lastLEDState != newState) digitalWrite(LED_PIN, lastLEDState = newState);
    break;
  case LED_RACE_ARMED_ACTIVE:
    if (lastLEDState) digitalWrite(LED_PIN, lastLEDState = LOW);
    break;
  case LED_HALF_RESET:
    newState = millis() % 333 < 167;
    if (lastLEDState != newState) digitalWrite(LED_PIN, lastLEDState = newState);
    break;
  case LED_HALF_RESET_PRESSED:
    if (!lastLEDState) digitalWrite(LED_PIN, lastLEDState = HIGH);
    break;
  case LED_HALF_RESET_DANGER:
    newState = millis() % 333 < 167;
    if (lastLEDState != newState) digitalWrite(LED_PIN, lastLEDState = newState);
    break;
  case LED_NO_WIFI:
    newState = millis() % 333 < 167;
    if (lastLEDState != newState) digitalWrite(LED_PIN, lastLEDState = newState);
    break;
  default:
    break;
  }
}