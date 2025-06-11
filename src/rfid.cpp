// rfid.cpp

#include <HardwareSerial.h>
#include <LittleFS.h>
#include "rfid.h"
#include "config.h"
#include "logs.h"

// Objeto da porta serial para o leitor RFID
HardwareSerial rfidSerial(2);

// Função auxiliar interna para ler a tag do RDM6300
String readRFID() {
  String rfidTag = "";
  if (rfidSerial.available() > 0) { // Verifica se há dados disponíveis
    char startByte = rfidSerial.read();
    Serial.print(startByte); // Imprime o byte lido (início)
    if (startByte == 0x02) { // Verifica byte de início (0x02)
      while (rfidSerial.available() < 10) { // Aguarda 10 bytes do ID
        delay(10); // Pequeno delay para esperar os dados
      }
      for (int i = 0; i < 10; i++) { // Lê 10 bytes do ID
        char c = rfidSerial.read();
        rfidTag += c; // Concatena cada caractere ao ID
        Serial.print(c); // Imprime cada caractere lido
      }
      char checksum1 = rfidSerial.read(); // Lê 2 bytes do checksum
      Serial.print(checksum1); // Imprime o primeiro byte do checksum
      char checksum2 = rfidSerial.read(); // Lê o segundo byte do checksum
      Serial.print(checksum2); // Imprime o segundo byte do checksum
      char endByte = rfidSerial.read(); // Lê byte de fim
      Serial.print(endByte); // Imprime o byte de fim
      if (endByte == 0x03) { // Verifica byte de fim (0x03)
        return rfidTag; // Retorna a tag lida
      }
    }
  }
  return ""; // Retorna vazio se não houver tag válida
}

// Função auxiliar interna para o fluxo de acesso
static void handleRFIDAccess() {
  String rfid = readRFID();
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