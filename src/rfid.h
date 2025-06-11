// rfid.h

#ifndef RFID_H
#define RFID_H

#include <Arduino.h>

void initRFID();
String readRFIDTag();
bool isRFIDRegistered(String tag);
bool addRFID(String tag);
bool removeRFID(String tag);
void handleRFIDAccess();

#endif // RFID_H