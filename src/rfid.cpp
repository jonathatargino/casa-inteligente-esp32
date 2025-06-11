// rfid.cpp

#include <HardwareSerial.h>
#include <LittleFS.h>
#include "rfid.h"
#include "config.h"
#include "logs.h"

HardwareSerial rfidSerial(2);

void initRFID() {
  rfidSerial.begin(9600, SERIAL_8N1, RFID_RX_PIN, RFID_TX_PIN);
  pinMode(RELE_PIN, OUTPUT);
  digitalWrite(RELE_PIN, LOW);
}

String readRFIDTag() {
  String rfidTag = "";
  if (rfidSerial.available() > 0) {
    if (rfidSerial.read() == 0x02) { // Start byte
      byte buffer[12];
      rfidSerial.readBytes(buffer, 12); // Read tag + checksum
      if (buffer[11] == 0x03) { // End byte
        for (int i = 0; i < 10; i++) {
          rfidTag += (char)buffer[i];
        }
      }
    }
  }
  return rfidTag;
}

bool isRFIDRegistered(String tag) {
  if (tag.isEmpty()) return false;
  File file = LittleFS.open(RFID_FILENAME, "r");
  if (!file) return false;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line == tag) {
      file.close();
      return true;
    }
  }
  file.close();
  return false;
}

bool addRFID(String tag) {
  if (tag.isEmpty() || isRFIDRegistered(tag)) return false;
  File file = LittleFS.open(RFID_FILENAME, FILE_APPEND);
  if (!file) return false;
  file.println(tag);
  file.close();
  return true;
}

bool removeRFID(String tagToRemove) {
  if (tagToRemove.isEmpty()) return false;
  File file = LittleFS.open(RFID_FILENAME, "r");
  if (!file) return false;
  String tempContent = "";
  bool found = false;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line == tagToRemove) {
      found = true;
    } else {
      tempContent += line + "\n";
    }
  }
  file.close();

  if (found) {
    file = LittleFS.open(RFID_FILENAME, "w");
    if (!file) return false;
    file.print(tempContent);
    file.close();
  }
  return found;
}

void handleRFIDAccess() {
  String rfid = readRFIDTag();
  if (rfid != "") {
    if (isRFIDRegistered(rfid)) {
      digitalWrite(RELE_PIN, HIGH);
      addLog(ACCESS_LOGS_FILENAME, "Acesso liberado para RFID: " + rfid);
      delay(2000);
      digitalWrite(RELE_PIN, LOW);
    } else {
      addLog(ACCESS_LOGS_FILENAME, "Acesso negado para RFID: " + rfid);
      delay(2000); // Pequeno delay para evitar logs repetidos
    }
  }
}