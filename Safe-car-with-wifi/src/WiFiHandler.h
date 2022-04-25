///@file WiFiHandler.h functions for wifi communication to clients

#include <Arduino.h>
#include "WebSocketsServer.h"

/*! @brief broadcast a message to all connected WebSocket clients
 * 
 * @param text the text to trasmit
 * @param justActive default: false, whether to just send it to active clients (in the app)
 * @param del default: 0, delay between individual messages
 * @param noPrint default: false, whether to avoid printnig the message to Serial
 */
void broadcastWSMessage(String text, bool justActive = false, int del = 0, bool noPrint = false);

/*! @brief broadcasting binary data to multiple devices
 * 
 * @param data pointer to a region in memory to send
 * @param length number of bytes to send
 * @param justActive whether to just send it to active (aka non-standby) devices
 * @param del delay in ms between individual messages
*/
void broadcastWSBin(uint8_t* data, size_t length, bool justActive = false, int del = 0);

/**
 * @brief function to be called on a WebSocket event
 * 
 * don't manually call it!
 * @param clientNo the spot of the client
 * @param type the event type
 * @param payload pointer to the data/payload
 * @param length length of the data/payload
 */
void onWebSocketEvent(uint8_t clientNo, WStype_t type, uint8_t * payload, size_t length);

/*! @brief routine after a client disconnects
 * 
 * @param spot the client that disconnected
*/
void removeClient (int spot);

/*! @brief routine after a client connects
 * 
 * sends current status to the client, LEDs, blocked UI elements and so on
 * @param spot the spot to send current status to
*/
void addClient (int spot);

/*! @brief send a message to a WebSocket client
 * 
 * @param spot the spot to send the message to, 255 for Serial
 * @param text the text to send
*/
void sendWSMessage(uint8_t spot, String text);

/*! @brief reconnect to wifi
 * 
 * disables ESC telemetry while reconnecting
*/
void reconnect();

/*! @brief checks the LED status
 *
 * transmits new LED status to connected clients if neccessary
 */
void checkLEDs();

/**
 * @brief sends the telemetry to all connected phones
 * 
 * telemetry format is documented in docs.md
 */
void sendTelemetry();

/*! @brief handles the wifi loop
 * 
 * runs WebSocket loop, checks LEDs and regularly sends telemetry to connected devices
*/
void handleWiFi();
