#include <Arduino.h>

void setArmed(bool arm, bool sendNoChangeBroadcast = false);
void setThrottle(double newThrottle);
uint16_t appendChecksum(uint16_t value, bool telemetryRequest = true);
void startRace();
double calcThrottle(int target, int was[], double additionalMultiplier = 1);
void receiveSerial();
float rpsToErpm(float rps);
float erpmToRps(float erpm);
void setNewTargetValue();
void sendRaceLog();
void evaluateThrottle();
void checkVoltage();