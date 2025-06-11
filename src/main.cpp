// SeuProjeto.ino

#include <Arduino.h>
#include <WiFi.h>
#include <FS.h>
#include <LittleFS.h>

// Inclui os cabeçalhos de todos os módulos.
// As declarações das tasks virão deles.
#include "config.h"
#include "logs.h"
#include "rfid.h"
#include "distancia.h"
#include "telegram.h"

// Função auxiliar para inicializar o sistema de arquivos
void initFS() {
  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
    Serial.println("Erro ao montar LittleFS");
    return;
  }
  
  // Garante que os arquivos de log e RFID existam
  File file;
  if (!LittleFS.exists(RFID_FILENAME)) {
    file = LittleFS.open(RFID_FILENAME, FILE_WRITE);
    if (file) file.close();
  }
  if (!LittleFS.exists(WATER_LOGS_FILENAME)) {
    file = LittleFS.open(WATER_LOGS_FILENAME, FILE_WRITE);
    if (file) file.close();
  }
  if (!LittleFS.exists(ACCESS_LOGS_FILENAME)) {
    file = LittleFS.open(ACCESS_LOGS_FILENAME, FILE_WRITE);
    if (file) file.close();
  }
}

// Função auxiliar para verificar o espaço no LittleFS
void checkLittleFSSpace() {
  size_t totalBytes = LittleFS.totalBytes();
  size_t usedBytes = LittleFS.usedBytes();
  Serial.println("Espaço total: " + String(totalBytes) + " bytes");
  Serial.println("Espaço usado: " + String(usedBytes) + " bytes");
}


void setup() {
  Serial.begin(9600);
  Serial.println("Iniciando sistema com Multi-Tasking (organizado)...");

  initFS();
  checkLittleFSSpace();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado. IP: " + WiFi.localIP().toString());

  // Inicializa os módulos
  initRFID();
  initDistancia();
  initTelegram();

  // --- Criação das Tasks ---
  // O `setup` agora apenas chama as funções de task que foram declaradas nos .h
  
  // Task do Sensor de Distância no Core 0
  xTaskCreatePinnedToCore(sensorTask, "SensorTask", 4096, NULL, 1, NULL, 0);
  Serial.println("Task do Sensor criada no Core 0.");

  // Task para o Leitor RFID no Core 1
  xTaskCreatePinnedToCore(rfidTask, "RFIDTask", 4096, NULL, 1, NULL, 1);
  Serial.println("Task do RFID criada no Core 1.");

  // Task para o Bot do Telegram no Core 1
  xTaskCreatePinnedToCore(telegramTask, "TelegramTask", 8192, NULL, 1, NULL, 1);
  Serial.println("Task do Telegram criada no Core 1.");

  Serial.println("Setup concluído. O sistema está operacional.");
}

void loop() {
  // O loop principal está corretamente vazio. O sistema agora é 100% orientado a tasks.
}