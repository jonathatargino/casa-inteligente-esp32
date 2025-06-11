// logs.cpp

#include <LittleFS.h>
#include "logs.h"

bool addLog(const char* filename, String log) {
  if (log.isEmpty()) return false;
  File file = LittleFS.open(filename, FILE_APPEND);
  if (!file) {
    Serial.println("Erro ao abrir arquivo para log: " + String(filename));
    return false;
  }

  unsigned long currentMillis = millis();
  unsigned long seconds = currentMillis / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  unsigned long days = hours / 24;

  currentMillis %= 1000;
  seconds %= 60;
  minutes %= 60;
  hours %= 24;

  String timestamp = String(days) + "-" +
                     (hours < 10 ? "0" : "") + String(hours) + ":" +
                     (minutes < 10 ? "0" : "") + String(minutes) + ":" +
                     (seconds < 10 ? "0" : "") + String(seconds);

  String logEntry = "[" + timestamp + "] " + log;
  
  if (file.println(logEntry)) {
    Serial.println("Log gravado: " + logEntry);
  } else {
    Serial.println("Erro ao gravar log no arquivo: " + String(filename));
  }
  
  file.close();
  return true;
}

String readLastLogs(const char* filename, int quantity) {
  File file = LittleFS.open(filename, "r");
  if (!file || file.size() == 0) {
    return "Nenhum log encontrado.";
  }

  String* lines = new String[quantity];
  int lineCount = 0;
  String currentLine = "";
  long pos = file.size() - 1;

  while (pos >= 0 && lineCount < quantity) {
    file.seek(pos);
    char c = file.read();

    if (pos == file.size() - 1 && c == '\n') {
      pos--;
      continue;
    }

    if (c == '\n' || pos == 0) {
      if (pos == 0) currentLine = c + currentLine;
      lines[lineCount++] = currentLine;
      currentLine = "";
    } else {
      currentLine = c + currentLine;
    }
    pos--;
  }
  
  file.close();

  String result = "";
  for (int i = 0; i < lineCount; i++) {
    result += lines[i] + "\n";
  }

  delete[] lines;
  return result;
}