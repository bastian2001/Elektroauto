#include <Arduino.h>

void setArmed(bool arm, bool sendBroadcast = false);
void setThrottle(double newThrottle);
uint16_t appendChecksum(uint16_t value);
void startRace();
double calcThrottle(int target, int was[]);
void receiveSerial();