// logs.h

#ifndef LOGS_H
#define LOGS_H

#include <Arduino.h>

bool addLog(const char* filename, String log);
String readLastLogs(const char* filename, int quantity);

#endif // LOGS_H