#include <Arduino.h>

void setArmed(bool arm);
void setThrottle(double newThrottle);
uint16_t appendChecksum(uint16_t value, bool telemetryRequest = true);
void startRace();
double calcThrottle(int target, int was[], double masterMultiplier = 1);
void receiveSerial();
float rpsToErpm(float rps);
float erpmToRps(float erpm);
void setNewTargetValue();
void sendRaceLog();
void throttleRoutine();
void checkVoltage();