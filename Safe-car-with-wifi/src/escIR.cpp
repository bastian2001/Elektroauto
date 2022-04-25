#include "global.h"

uint32_t escOutputCounter = 0;

void IRAM_ATTR escIR() {
  sendESC = true;
}