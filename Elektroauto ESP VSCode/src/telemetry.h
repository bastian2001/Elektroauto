/**
 * @brief acquires telemetry data
 * 
 * reads telemetry data from Serial2 buffer
 * converts the values to a readable format
 * checks for overheating/cutoffvoltage/too high rpm
 * sets LEDs to green only after bootup
 */
void getTelemetry();

/**
 * @brief sends the telemetry to all connected phones
 * 
 * telemetry format is documented in docs.md
 */
void sendTelemetry();

/**
 * @brief checks if telemetry is ready to be processed
 * 
 * @return whether telemetry can be read
 */
bool isTelemetryComplete();

//! @brief updates the crc checksum
uint8_t update_crc8(uint8_t crc, uint8_t crc_seed);

/**
 * @brief Get the crc checksum of given data
 * 
 * @param Buf pointer to a memory area to create the checksum of
 * @param BufLen the buffer length
 * @return the checksum 
 */
uint8_t get_crc8(uint8_t *Buf, uint8_t BufLen);