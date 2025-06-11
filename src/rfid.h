// rfid.h

#ifndef RFID_H
#define RFID_H

#include <Arduino.h>

// Funções do módulo
void initRFID();
bool isRFIDRegistered(String tag);
bool addRFID(String tag);
bool removeRFID(String tag);

// Declaração da Task do RFID
void rfidTask(void* parameter);

#endif // RFID_H