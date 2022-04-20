#include "global.h"
#include "lightSensor.h"
#include "system.h"
bool lightSensorStart = false;

void IRAM_ATTR onLightSensorChange(int previousStatus, int newStatus){
	if (newStatus == LS_STATE_START){
		// startRace();
		lightSensorStart = true;
	}
}