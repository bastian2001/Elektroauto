#include "lightSensor.h"
#include <Arduino.h>

int ringBufPos = 0, lastRingBufPos = 3;
int pos;
int lsPin;
void (*lsStateChange) (int previousStatus, int newStatus);

typedef struct lightEvent{
    unsigned long risingEdge = 0; ///micros() of rising edge
    unsigned long duration = 0; ///micros() of falling edge
	unsigned int distance = 0;
} LightEvent;
///stores previous light sensor events
LightEvent previousLightEvents[4];
unsigned long lastRisingEdge;
unsigned long lastValidState;
int lsState = 0;
int counterAvailable, counterArmed, counterRace;

void IRAM_ATTR lightSensorRoutine(){
	if (digitalRead(lsPin)){
		lastRisingEdge = micros();
	} else {
		ringBufPos++;
		lastRingBufPos++;
		if (ringBufPos == 4) ringBufPos = 0;
		previousLightEvents[ringBufPos].risingEdge = lastRisingEdge;
		previousLightEvents[ringBufPos].duration = micros() - lastRisingEdge;
		previousLightEvents[ringBufPos].distance = lastRisingEdge - previousLightEvents[lastRingBufPos].risingEdge;

		if (lastValidState < millis() - 20 && lsState != NO_BLOCK){
			int temp = lsState;
			lsState = NO_BLOCK;
			if (lsStateChange != nullptr)
				lsStateChange(temp, NO_BLOCK);
		}
		for (pos = 0; pos < 4; pos++){
			if (previousLightEvents[pos].distance > START_BLOCK_MICROS_MAX || previousLightEvents[pos].distance < START_BLOCK_MICROS_MIN)
				return;
		}
		counterAvailable = counterArmed = counterRace = 0;
		for (pos = 0; pos < 4; pos++){
			int dur = previousLightEvents[pos].duration;
			if (dur > 1000000 / START_BLOCK_FREQ / 4 - 40 && dur < 1000000 / START_BLOCK_FREQ / 4 + 40){
				//short pulse - block is present
				counterAvailable++;
			} else if (dur > 1000000 / START_BLOCK_FREQ / 2 - 40 && dur < 1000000 / START_BLOCK_FREQ / 2 + 40){
				//middle pulse - block is armed
				counterArmed++;
			} else if (dur > 1000000 / START_BLOCK_FREQ * 3 / 4 - 40 && dur < 1000000 / START_BLOCK_FREQ * 3 / 4 + 40){
				//loong pulse - start race
				counterRace++;
			} else return; //invalid duration
		}
		if (counterAvailable && counterArmed && counterRace) return; //all three modes present -> shouldn't be
		lastValidState = millis();
		if (counterAvailable >= 3){
			int temp = lsState;
			lsState = START_BLOCK_DETECTED;
			if (lsStateChange != nullptr)
				lsStateChange(temp, START_BLOCK_DETECTED);
		}
		else if (counterArmed >= 3 && (lsState == START_BLOCK_DETECTED || lsState == NO_BLOCK)) {
			int temp = lsState;
			lsState = START_BLOCK_ARMED;
			if (lsStateChange != nullptr)
				lsStateChange(temp, START_BLOCK_ARMED);
		}
		else if (counterRace >= 3 && lsState == START_BLOCK_ARMED) {
			lsState = START_BLOCK_RACE;
			if (lsStateChange != nullptr)
				lsStateChange(START_BLOCK_ARMED, START_BLOCK_RACE);
		}
	}
}

int * initLightSensor(int pin, void (* onStatusChange) (int previousStatus, int newStatus)){
	pinMode(pin, INPUT_PULLUP);
	attachInterrupt(pin, &lightSensorRoutine, CHANGE);
	lsPin = pin;
	lsStateChange = onStatusChange;
	return &lsState;
}