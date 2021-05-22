#include <Arduino.h>
#include <WebSocketsServer.h>
#include "driver/rmt.h"


/*======================================================definitions======================================================*/

//Pin numbers
#define ESC_OUTPUT_PIN 25
#define ESC_TRIGGER_PIN 33
#define TRANSMISSION 23
#define LED_BUILTIN 22

//ESC values
#define ESC_FREQ 1000
#define CLK_DIV 3 //DShot 150: 12, DShot 300: 6, DShot 600: 3
#define TT 44 // total bit time
#define T0H 17 // 0 bit high time
#define T1H 33 // 1 bit high time
#define T0L (TT - T0H) // 0 bit low time
#define T1L (TT - T1H) // 1 bit low time
#define T_RESET 21 // reset length in multiples of bit time
#define TRANSMISSION_IND 1000
// #define SEND_TRANSMISSION_IND //whether to enable or disable the transmission indicator overall
#define ESC_BUFFER_ITEMS 16

//motor and wheel properties
#define TELEMETRY_DEBUG 3
#define MAX_THROTTLE 2000
#define MAX_TARGET_RPS 1500
#define MAX_TARGET_SLIP 20
// #define SEND_TRANSMISSION_IND
#define ESC_BUFFER_ITEMS 16
#define MOTOR_POLE_COUNT 12.0f
#define WHEEL_DIAMETER 30.0f
#define RPS_CONVERSION_FACTOR (1.6667f / (MOTOR_POLE_COUNT / 2.0f))
#define ERPM_TO_MM_PER_SECOND (RPS_CONVERSION_FACTOR * WHEEL_DIAMETER * PI)

//WiFi and WebSockets settings
#define MAX_WS_CONNECTIONS 5
// #define ssid "Bloedfrauen und -maenner"
// #define password "CaputDraconis"
#define ssid "POCO X3 NFC"
#define password "055fb39a4cc4"
// #define ssid "springernet"
// #define password "CL7B1L609235"
#define TELEMETRY_UPDATE_MS 20
#define TELEMETRY_UPDATE_ADD 20

//debugging settings
#define PRINT_SETUP
// #define PRINT_TELEMETRY_THROTTLE
// #define PRINT_TELEMETRY_TEMP
// #define PRINT_TELEMETRY_ERPM
// #define PRINT_TELEMETRY_VOLTAGE
#define PRINT_WEBSOCKET_CONNECTIONS
#define PRINT_INCOMING_MESSAGES
#define PRINT_MEANINGS
#define PRINT_BROADCASTS
// #define PRINT_SINGLE_OUTGOING_MESSAGES
// #define PRINT_RACE_MODE_JSON

//PID loop settings
#define TREND_AMOUNT 5 //nur ungerade!!
#define TA_DIV_2 2 //mit anpassen!!

//logging settings
#define LOG_FRAMES 5000



struct Action {
    uint8_t action; //0 empty, 1 disarm, 2 arm
    uint8_t payload;
    uint32_t millis;
};

//rps control variables
extern int targetERPM;
extern int previousERPM[TREND_AMOUNT];
extern double pidMulti;
extern double erpmA; //große Änderungen: zu viel -> overshooting bei großen Anpassungen, abwürgen. Zu wenig -> langsames Anpassen bei großen Änderungen
extern double erpmB;
extern double erpmC; //responsiveness: zu viel -> schnelles wackeln um den eigenen Wert, evtl. overshooting bei kleinen Anpassungen. zu wenig -> langsames Anpassen bei kleinen Änderungen

//MPU variables
//MPU6050 mpu;
extern unsigned long lastMPUUpdate;
extern int16_t counterMPU, MPUoffset, raw_accel;
extern int16_t distMPU;
extern double speedMPU, acceleration;

//slip variables
extern int targetSlip;
extern uint16_t speedWheel;

//system variables
extern double throttle, nextThrottle;
extern bool armed;
extern int ctrlMode, reqValue;
extern uint16_t escValue;
extern uint16_t cutoffVoltage, voltageWarning;
extern bool redLED, greenLED, blueLED;
extern uint8_t newRedLED, newGreenLED, newBlueLED;

//WiFi/WebSockets variables
extern WebSocketsServer webSocket;
extern uint8_t clients[MAX_WS_CONNECTIONS][2]; //[device (disconnected, app, web)][telemetry (off, on)]
extern uint8_t telemetryClientsCounter;

//ESC variables
extern uint16_t telemetryERPM;
extern uint8_t telemetryTemp;
extern uint16_t telemetryVoltage;
extern uint16_t errorCount;
extern uint8_t manualDataAmount;
extern uint16_t manualData[20];

//race mode variables
extern uint16_t throttle_log[LOG_FRAMES], erpm_log[LOG_FRAMES], voltage_log[LOG_FRAMES];
extern int acceleration_log[LOG_FRAMES];
extern uint8_t temp_log[LOG_FRAMES];
extern uint16_t logPosition;
extern bool raceModeSendValues, raceMode, raceActive;


void sPrint(String s);
void sPrintln(String s);
void printSerial();