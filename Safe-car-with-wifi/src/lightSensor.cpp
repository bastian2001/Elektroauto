#include "lightSensor.h"
#include <Arduino.h>
#include "global.h"
#include "LED.h"


// int ringBufPos = 0, lastRingBufPos = 3;
// int pos;
int previousReading, currentReading;
int lsPin;
void (*lsStateChange) (int previousState, int newState);

#define LS_BUFFER_LENGTH 4
// unsigned long lastLightOff[LS_BUFFER_LENGTH], lastLightOn[LS_BUFFER_LENGTH];
unsigned long lastLightOff, lastLightOn;
int lsState = 0;
uint32_t lastStateChange;
int counterAvailable, counterArmed, counterRace;

void IRAM_ATTR startState(){
	lsState = LS_STATE_START;
	lsStateChange(LS_STATE_READY, LS_STATE_START);
	lastStateChange = millis();
	detachInterrupt(lsPin);
}

void lsLoop(){
	uint32_t now = millis();
	currentReading = digitalRead(lsPin);
	// for (int i = LS_BUFFER_LENGTH - 1; i > 0 && !currentReading; i--){
	// 	lastLightOn[i] = lastLightOn[i-1];
	// }
	// for (int i = LS_BUFFER_LENGTH - 1; i > 0 && currentReading; i--){
	// 	lastLightOff[i] = lastLightOff[i-1];
	// }

	int lightChange = (previousReading && !currentReading) - (currentReading && !previousReading);

	switch (lsState){
		case LS_STATE_UNKNOWN:
			if (lightChange == -1 && now - lastLightOn > 40 && now - lastLightOn < 60){
				lsState = LS_STATE_IDLE;
				lsStateChange(LS_STATE_UNKNOWN, LS_STATE_IDLE);
				if (statusLED == LED_RACE_MODE) setStatusLED(LED_FOUND_BLOCK);
				lastStateChange = now;
			}
			break;
		case LS_STATE_IDLE:
			if (lightChange == 1){
				// off -> on
				if (now - lastLightOff < 40 || now - lastLightOff > 60){
					lsState = LS_STATE_UNKNOWN;
					if (statusLED == LED_FOUND_BLOCK) resetStatusLED();
					lsStateChange(LS_STATE_IDLE, LS_STATE_UNKNOWN);
					lastStateChange = now;
				}
			}
			if (lightChange == -1){
				//on -> off
				if (now - lastLightOn > 60){
					lsState = LS_STATE_UNKNOWN;
					if (statusLED == LED_FOUND_BLOCK) resetStatusLED();
					lsStateChange(LS_STATE_IDLE, LS_STATE_UNKNOWN);
					lastStateChange = now;
				}
			}
			if (now - lastLightOff > 120 && lastLightOff - lastStateChange <= 500){
				lsState = LS_STATE_UNKNOWN;
				if (statusLED == LED_FOUND_BLOCK) resetStatusLED();
				lsStateChange (LS_STATE_IDLE, LS_STATE_UNKNOWN);
				lastStateChange = now;
			}
			if (currentReading && now - lastLightOff >= 2000 && lastLightOff - lastStateChange > 500){
				lsState = LS_STATE_READY;
				lsStateChange (LS_STATE_IDLE, LS_STATE_READY);
				if (statusLED == LED_FOUND_BLOCK) setStatusLED(LED_BLOCK_READY);
				lastStateChange = now;
				attachInterrupt (lsPin, &startState, FALLING);
			}
			break;
		case LS_STATE_READY:
			if (now - lastStateChange > 300000){
				lsState = LS_STATE_UNKNOWN;
				if (statusLED == LED_BLOCK_READY) resetStatusLED();
				lsStateChange (LS_STATE_READY, LS_STATE_UNKNOWN);
				lastStateChange = now;
				detachInterrupt(lsPin);
			}
			break;
		case LS_STATE_START:
			if (currentReading){
				lsState = LS_STATE_UNKNOWN;
				lsStateChange(LS_STATE_START, LS_STATE_UNKNOWN);
				lastStateChange = now;
				if (statusLED == LED_RACE_ARMED_ACTIVE) resetStatusLED();
			}
			break;
	}

	previousReading = currentReading;
	if (lightChange == 1){
		lastLightOn = now;

	}
	if (lightChange == -1){
		lastLightOff = now;

	}
}

hw_timer_t * timerLS;
int * initLightSensor(int pin, void (* onStatusChange) (int previousStatus, int newStatus), bool enableAutoLoop){
	pinMode(pin, INPUT_PULLUP);
	timerLS = timerBegin(1, 80, true);
	timerAttachInterrupt(timerLS, &lsLoop, true);
	timerAlarmWrite(timerLS, 1000000/LS_FREQ, true);
	timerAlarmEnable(timerLS);
	lsPin = pin;
	lsStateChange = onStatusChange;
	return &lsState;
}

void pauseLS(){
	timerAlarmDisable(timerLS);
}

void resumeLS(){
	timerAlarmEnable(timerLS);
}