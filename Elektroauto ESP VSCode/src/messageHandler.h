//! @file messageHandler.h function for message handling

#include <Arduino.h>

/*! @brief processes Serial- and WebSocket messages
 * 
 * Message more precisely documented in docs.md
 * - VALUE command for requesting a value
 * - ARMED command for arming the car
 * - PING command for replying with PONG / pinging the connection
 * - MODE command for setting the driving mode
 * - TELEMETRY command for setting telemetry of the origin on or off
 * - DEVICE command for setting the device type
 * - RACEMODE command for enabling or disabling race mode
 * - STARTRACE command for starting the race
 * - CUTOFFVOLTAGE, WARNINGVOLTAGE, RPSA, RPSB, RPSC and PIDMULTIPLIER command for setting their respective parameters 
 * - ERRORCOUNT for requesting the error count
 * - RECONNECT for reconnecting to WiFi
 * - RAWDATA for sending raw data to the ESC
 * @param message The message
 * @param from The origin of the message, 255 for Serial, other values for WebSocket spots
*/
void processMessage(String message, uint8_t from);