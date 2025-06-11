// rfid.cpp

#include <HardwareSerial.h>
#include <LittleFS.h>
#include "rfid.h"
#include "config.h"
#include "logs.h"

// Objeto da porta serial para o leitor RFID
HardwareSerial rfidSerial(2);

// Função auxiliar interna para ler a tag do RDM6300
static String readRFIDTag() {
  String rfidTag = "";
  if (rfidSerial.available() > 0) {
    if (rfidSerial.read() == 0x02) { // Byte de início (STX)
      byte buffer[12]; // 10 bytes de dados + 2 bytes de checksum
      rfidSerial.readBytes(buffer, 12);
      
      // O último byte lido deve ser o byte de fim (ETX)
      if (buffer[11] == 0x03) { 
        // Converte os 10 bytes de dados hexadecimais em uma string
        for (int i = 0; i < 10; i++) {
          rfidTag += (char)buffer[i];
        }
      }
    }
  }
  return rfidTag;
}

// Função auxiliar interna para o fluxo de acesso
static void handleRFIDAccess() {
  String rfid = readRFIDTag();
  if (rfid != "") {
    if (isRFIDRegistered(rfid)) {
      digitalWrite(RELE_PIN, HIGH);
      addLog(ACCESS_LOGS_FILENAME, "Acesso liberado para RFID: " + rfid);
      vTaskDelay(2000 / portTICK_PERIOD_MS); 
      digitalWrite(RELE_PIN, LOW);
    } else {
      addLog(ACCESS_LOGS_FILENAME, "Acesso negado para RFID: " + rfid);
      vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
  }
}


// --- Funções Públicas ---

void initRFID() {
  rfidSerial.begin(9600, SERIAL_8N1, RFID_RX_PIN, RFID_TX_PIN);
  pinMode(RELE_PIN, OUTPUT);
  digitalWrite(RELE_PIN, LOW);
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

void rfidTask(void* parameter) {
  Serial.println("Iniciando loop da Task de RFID...");
  while (true) {
    handleRFIDAccess();
    vTaskDelay(50 / portTICK_PERIOD_MS); 
  }
}