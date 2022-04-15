#include "global.h"
#include "lightSensor.h"
#include "system.h"

void IRAM_ATTR onLightSensorChange(int previousStatus, int newStatus){
	if (newStatus == LS_STATE_START){
		startRace();
		// Serial.println("race started");
	}
}