/**
 * @brief loop on core 0
 * 
 * the not so time-sensitive stuff
 * initiates:
 * - wifi reconnection if neccessary
 * - race mode values sending
 * - low voltage checking
 * - wifi and serial message handling
 * - Serial string printing
 */
void loop0();

/**
 * @brief loop on core 1
 * 
 * time sensitive stuff
 * initiates:
 * - (MPU checking)
 * - telemetry acquisition and processing
 * - throttle routine
 */
void loop();

//! @brief Task for core 0, creates loop0
void core0Code();

/**
 * @brief setup function
 * 
 * enables Serial communication
 * connects to wifi
 * sets up pins and timer for escir
 * creates task on core 0
 * initates websocket server
 */
void setup();