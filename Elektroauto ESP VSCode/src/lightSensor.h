///@file lightSensor.h Provides functions for the light sensor
#include <Arduino.h>

///frequency of start block
#define START_BLOCK_FREQ 1200
///tolerated offset
#define START_BLOCK_FREQ_DELTA 30
#define START_BLOCK_FREQ_MAX START_BLOCK_FREQ+START_BLOCK_FREQ_DELTA
#define START_BLOCK_FREQ_MIN START_BLOCK_FREQ-START_BLOCK_FREQ_DELTA
#define START_BLOCK_MICROS_MAX 1000000/START_BLOCK_FREQ_MIN
#define START_BLOCK_MICROS_MIN 1000000/START_BLOCK_FREQ_MAX


enum LighSensorStates{
	NO_BLOCK = 0,
	START_BLOCK_DETECTED,
	START_BLOCK_ARMED,
	START_BLOCK_RACE
};

/**
 * @brief runs when the light sensor changed its value
 * 
 * on rising edge: stores a timestamp<br>
 * on falling edge: determines the duration and compares it to previous events. If the race mode is active and the start block is armed, it may start the race
 */
void IRAM_ATTR lightSensorRoutine();

/**
 * @brief initialises the light sensor
 * 
 * @param pin the pin of the light sensor
 * @param onStatusChange method that is called when the status changes
 * @return int* the current status
 */
int * initLightSensor(int pin, void (* onStatusChange) (int previousStatus, int newStatus));