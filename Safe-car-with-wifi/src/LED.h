///@file LED.h LED manager

/**
 * @brief sets the status LED pattern
 * 
 * takes into account the priority of the statuses
 * 
 * @param newStatus the new status
 * @return whether the status changed
 */
bool setStatusLED (int newStatus);

/**
 * @brief resets the status LED
 * 
 * no checks
 */
void resetStatusLED();

/**
 * @brief drives the LED
 * 
 */
void ledRoutine();