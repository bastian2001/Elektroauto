//! @file settings.h provides functions for reading and writing settings
#include "Arduino.h"

bool firstStartup();

void writeEEPROM();

void readEEPROM();

void clearEEPROM();

void sendSettings(uint8_t to);