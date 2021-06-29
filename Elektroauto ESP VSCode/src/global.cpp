//! @file global.cpp global functions

#include <Arduino.h>
#include "global.h"

String toBePrinted = "";

/** 
 * @brief Print the current Serial string
 */
void printSerial() {
  Serial.print(toBePrinted);
  toBePrinted = "";
}

/** 
 * @brief add anything to the Serial string
 * @param s the String to print
 */
void sPrint(String s) {
  toBePrinted += s;
}

/** 
 * @brief add a line to the Serial string
 * @param s the String to print
 */
void sPrintln(String s) {
  toBePrinted += s;
  toBePrinted += "\n";
}