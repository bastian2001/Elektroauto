//! @file messageHandler.h function for message handling

#include <Arduino.h>

/*! @brief processes Serial- and WebSocket messages
 * 
 * Message more precisely documented in docs.md or messageHandler.cpp documentation
 * @param message The message
 * @param from The origin of the message, 255 for Serial, other values for WebSocket spots
*/
void processMessage(String message, uint8_t from);