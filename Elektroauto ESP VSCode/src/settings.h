//! @file settings.h provides functions for reading and writing settings
#include "Arduino.h"

/**
 * @brief checks if this is the first boot for this controller
 * @return true if this is the first startup
 */
bool firstStartup();

/**
 * @brief writes the settings to the EEPROM
 */
void writeEEPROM();

/**
 * @brief reads the settings from the EEPROM
 */
void readEEPROM();

/**
 * @brief restores the defaults, i.e. sets the firstStartup byte to 0
 */
void clearEEPROM();

/**
 * @brief sends the settings to a client
 * @param to the client number, 255 for Serial
 */
void sendSettings(uint8_t to);