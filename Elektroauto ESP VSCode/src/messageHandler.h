#include <Arduino.h>

/*! @brief processes Serial- and WebSocket messages
 * 
 * Message more precisely documented in docs.md
 * @param message The message
 * @param from The origin of the message, 255 for Serial, other values for WebSocket spots
*/
void dealWithMessage(String message, uint8_t from);