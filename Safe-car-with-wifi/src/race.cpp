#include "global.h"
#include "lightSensor.h"
#include "system.h"
bool lightSensorStart = false;

void IRAM_ATTR onLightSensorChange(int previousStatus, int newStatus){
	if (newStatus == LS_STATE_START){
		lightSensorStart = true;
	}
}

void setRaceThrottle(){
	if (raceMode && raceStopAt && automaticRace){
		uint32_t timePassed = millis() - (raceStopAt - 4000);
		if (timePassed < 1000){
			reqValue = 2000;
		} else if (timePassed < 2000){
			reqValue = 1400;
		} else if (timePassed < 3000){
			reqValue = timePassed * 2 / 5 + 600;
		} else {
			reqValue = 1800;
		}
		setNewTargetValue();
	}
}