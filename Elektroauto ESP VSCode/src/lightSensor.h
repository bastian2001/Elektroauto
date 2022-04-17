///@file lightSensor.h Provides functions for the light sensor
#include <Arduino.h>

/// light sensor update frequency
#define LS_FREQ 900


/// defines light sensor states
enum {
	LS_STATE_UNKNOWN,
    LS_STATE_IDLE,
    LS_STATE_READY,
    LS_STATE_START
};

/**
 * @brief runs when the light sensor changed its value
 * 
 * on rising edge: stores a timestamp<br>
 * on falling edge: determines the duration and compares it to previous events. If the race mode is active and the start block is armed, it may start the race
 */
void lsLoop();

/**
 * @brief initialises the light sensor
 * 
 * @param pin the pin of the light sensor
 * @param onStatusChange method that is called when the status changes
 * @return int* the current status
 */
int * initLightSensor(int pin, void (* onStatusChange) (int previousStatus, int newStatus), bool enableAutoLoop = true);

/// @brief pauses the light sensor timer
void pauseLS();

///@brief resumes the light sensor timer
void resumeLS();