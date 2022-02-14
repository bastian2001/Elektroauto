#include "global.h"
#include "system.h"
#include "settings.h"
#include "button.h"
#include "LED.h"
#include "wifiStuff.h"

bool lastButtonState = HIGH;

void onButtonDown(){
  setArmed(false);
  if (raceMode == true && raceActive == false){
    enableRaceMode(false);
  }
}

void onLongPress(){
  enableRaceMode(true);
}

void checkButton(){
  bool currentState = digitalRead(BUTTON_PIN);
  bool halfReset = (lastButtonEvent.time > millis() - 10000) && (lastButtonEvent.type == ButtonEventType::DoublePress);
  if (halfReset)
    setStatusLED(LED_HALF_RESET);
  if (currentState == LOW){
    //button pressed
    if(lastButtonState == HIGH){
      //button pressed just now
      for (int i = 0; i < 10; i++){
        //debounce: ignore false triggers
        delayMicroseconds(20);
        currentState = digitalRead(BUTTON_PIN);
        if (currentState)
          return;
      }
      lastButtonDown = millis();
      onButtonDown();
    } else {
      if (halfReset) setStatusLED(LED_HALF_RESET_PRESSED);
      if (lastButtonDown < millis() - 3000 && halfReset){ //very long press
        setStatusLED(LED_HALF_RESET_DANGER);
      }
      if (lastButtonDown < millis() - 5000){ //too long, even for very long pres
        if (statusLED >= LED_HALF_RESET && statusLED <= LED_HALF_RESET_DANGER)
          resetStatusLED();
      }
    }
  } else {
    //button not pressed
    if (lastButtonState == LOW){
      //button released just now
      //determine button press length
      unsigned int duration = millis() - lastButtonDown;
      if (duration > 50 && duration < 600){
        if (lastButtonEvent.time > millis() - 1000 && (lastButtonEvent.type == ButtonEventType::ShortPress)){
          //double press
          lastButtonEvent.type = ButtonEventType::DoublePress;
          lastButtonEvent.time = millis();
        }else {
          //short press
          lastButtonEvent.type = ButtonEventType::ShortPress;
          lastButtonEvent.time = millis();
          if (statusLED >= LED_HALF_RESET && statusLED <= LED_HALF_RESET_DANGER)
            resetStatusLED();
        }
      } else if (duration > 1000 && duration < 3000){
        lastButtonEvent.type = ButtonEventType::LongPress;
        lastButtonEvent.time = millis();
        onLongPress();
      } else if (halfReset && duration > 3000 && duration < 5000){
        clearEEPROM();
        delay(200);
        ESP.restart();
      } else {
        lastButtonEvent.type = ButtonEventType::ShortPress;
        lastButtonEvent.time = 0;
      }
    }
  }
  if (statusLED >= LED_HALF_RESET && statusLED <= LED_HALF_RESET_DANGER && lastButtonEvent.time < millis() - 10000)
      resetStatusLED();
  lastButtonState = currentState;
}

void enableRaceMode(bool en){
  if (en){
    broadcastWSMessage("SET RACEMODETOGGLE ON");
    broadcastWSMessage("UNBLOCK VALUE");
    setStatusLED(LED_RACE_MODE);
  } else {
    broadcastWSMessage("SET RACEMODETOGGLE OFF");
    if (!((ESCs[0]->status) & ARMED_MASK))
      broadcastWSMessage("BLOCK VALUE 0");
    if (statusLED == LED_RACE_MODE) resetStatusLED();
  }
  raceMode = en;
}