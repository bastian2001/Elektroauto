#include <Arduino.h>
#include "global.h"

String toBePrinted = "";

//! @brief prints out the Serial string to Serial buffer
void printSerial() {
  Serial.print(toBePrinted);
  toBePrinted = "";
}

//! @brief add anything to the Serial string
void sPrint(String s) {
  toBePrinted += s;
}

//! @brief add a line to the Serial string
void sPrintln(String s) {
  toBePrinted += s;
  toBePrinted += "\n";
}