//! @file global.h global functions and parameters

#include <Arduino.h>
#include <WebSocketsServer.h>
#include "driver/rmt.h"
#include <EEPROM.h>
#include "WiFi.h"
#include <esp_task_wdt.h>
#include "BMI160Gen.h"
#include "ESC.h"



/// pin used for output to ESC1
#define ESC1_OUTPUT_PIN 27
/// pin used for output to ESC2
#define ESC2_OUTPUT_PIN 26
/// pin of the built-in LED
#define LED_BUILTIN 22
/// pin used for LED
#define LED_PIN LED_BUILTIN
/// pin used for button control
#define BUTTON_PIN 4
/// pin used for the phototransistor
#define LIGHT_SENSOR_PIN 32


///frequency of basically everything
#define ESC_FREQ 4000


//WiFi and WebSockets settings
/// Maximum number of concurrent WebSocket connections
#define MAX_WS_CONNECTIONS 5
/// send out telemetry to clients every ... ms
#define TELEMETRY_UPDATE_MS 20
/// add ... ms to telemetry frequency for every connected client (1 client: Telemetry every TELEMETRY_UPDATE_MS + /TELEMETRY_UPDATE_ADD milliseconds)
#define TELEMETRY_UPDATE_ADD 20
/// The SSID
// #define ssid "Bloedfrauen und -maenner"
/// The password for wifi
// #define password "CaputDraconis"
#define ssid "POCO X3 NFC"
#define password "hallo123"
// #define ssid "springernet"
// #define password "CL7B1L609235"

//debugging settings
/// print setup info (e.g. IP-Address)
#define PRINT_SETUP
/// print info about WebSocket connections, e.g. new connections
#define PRINT_WEBSOCKET_CONNECTIONS
/// print incoming messages from WebSocket clients
#define PRINT_INCOMING_MESSAGES
/// print broadcasts into the Serial connection
#define PRINT_BROADCASTS

/// defines LED blinking behaviour
enum LEDModes {
    LED_OFF = 0,
    LED_SETUP,
    LED_NO_DEVICE,
    LED_NO_WIFI,
    LED_RACE_MODE,
    LED_FOUND_BLOCK,
    LED_RACE_ARMED_ACTIVE,
    LED_HALF_RESET,
    LED_HALF_RESET_PRESSED,
    LED_HALF_RESET_DANGER,
};
// the status represents a hirarchy, call resetStatusLED() to reset it to 0

///defines the types of button events
enum class ButtonEventType{
    ShortPress = 0,
    LongPress,
    DoublePress
};
/// A button event
typedef struct buttonEvent{
    ButtonEventType type = ButtonEventType::ShortPress;
    unsigned long time = 0;
} ButtonEvent;

/// stores the last button event (e.g. for double press actions)
extern ButtonEvent lastButtonEvent;
/// when the button was last pressed
extern unsigned long lastButtonDown;

/// status LED
extern int statusLED;



// EEPROM
/// don't change anything here, keeps track of whether/when to save the settings to EEPROM
extern bool commitFlag;

// control variables
extern int reqValue;

// race mode
extern bool raceMode;
extern uint32_t raceStopAt;
extern bool automaticRace;

// WiFi/WebSockets
extern WebSocketsServer webSocket;
extern uint8_t clients[MAX_WS_CONNECTIONS][2]; //[device (disconnected, app, web)][telemetry (off, on)]
extern uint8_t telemetryClientsCounter;

// ESCs
extern ESC *ESCs[2]; //0: left, 1: right

// light sensor
extern int * lsStatePtr;

extern bool sendESC;
extern uint8_t escPassthroughMode;


/** 
 * @brief add anything to the Serial string
 * @param s the String to print
 */
void sPrint(String s);

/** 
 * @brief add a line to the Serial string
 * @param s the String to print
 */
void sPrintln(String s);

void printSerial();