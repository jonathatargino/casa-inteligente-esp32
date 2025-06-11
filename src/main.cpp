// SeuProjeto.ino

#include <Arduino.h>
#include <WiFi.h>
#include <FS.h>
#include <LittleFS.h>

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
  
  // Garante que os arquivos existam
  File file;
  if (!LittleFS.exists(RFID_FILENAME)) {
    file = LittleFS.open(RFID_FILENAME, FILE_WRITE);
    file.close();
  }
  if (!LittleFS.exists(WATER_LOGS_FILENAME)) {
    file = LittleFS.open(WATER_LOGS_FILENAME, FILE_WRITE);
    file.close();
  }
  if (!LittleFS.exists(ACCESS_LOGS_FILENAME)) {
    file = LittleFS.open(ACCESS_LOGS_FILENAME, FILE_WRITE);
    file.close();
  }
}

void checkLittleFSSpace() {
  size_t totalBytes = LittleFS.totalBytes();
  size_t usedBytes = LittleFS.usedBytes();
  Serial.println("Espaço total: " + String(totalBytes) + " bytes");
  Serial.println("Espaço usado: " + String(usedBytes) + " bytes");
}


void setup() {
  Serial.begin(9600);
  Serial.println("Iniciando o sistema...");

  // 1. Inicializa o sistema de arquivos
  initFS();
  checkLittleFSSpace();

  // 2. Conecta ao Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado. IP: " + WiFi.localIP().toString());

  // 3. Inicializa os módulos
  initRFID();
  initDistancia();
  initTelegram();

  // 4. Cria a Task para o sensor de distância, rodando no Core 0
  xTaskCreatePinnedToCore(
    sensorTask,     // Função da Task
    "SensorTask",   // Nome da Task
    4096,           // Tamanho da pilha
    NULL,           // Parâmetros da Task
    1,              // Prioridade
    NULL,           // Handle da Task
    0               // Core (0 ou 1)
  );

  Serial.println("Setup concluído. O sistema está operacional.");
}

void loop() {
  // O Core 1 (padrão do Arduino) cuidará do RFID e do Telegram
  handleRFIDAccess();
  handleTelegramMessages();
}