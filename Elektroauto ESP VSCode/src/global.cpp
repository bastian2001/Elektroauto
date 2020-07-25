#include <Arduino.h>
#include "global.h"

String toBePrinted = "";

void printSerial() {
  Serial.print(toBePrinted);
  toBePrinted = "";
}

void sPrint(String s) {
  toBePrinted += s;
}

void sPrintln(String s) {
  toBePrinted += s;
  toBePrinted += "\n";
}