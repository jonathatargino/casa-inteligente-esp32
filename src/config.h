// config.h

#ifndef CONFIG_H
#define CONFIG_H

// -- Configurações Wi-Fi --
const char* WIFI_SSID = "sequestro";
const char* WIFI_PASSWORD = "estouapto";

// -- Configurações Telegram --
#define BOT_TOKEN "7344592024:AAGDK4Mu6kvV9U37gf_R8C8u32dgHDR5KEA"
#define CHAT_ID "7620344992"

// -- Configurações de Pinos --
#define RFID_RX_PIN 16
#define RFID_TX_PIN 17
#define DISTANCIA_TRIG_PIN 5
#define DISTANCIA_ECHO_PIN 18
#define LED_NIVEL_PIN 4
#define RELE_PIN 14

// -- Configurações do Sensor de Distância --
#define SOUND_SPEED 0.034
#define VOLUME_AGUA_ALTO 70  // Distância que representa nível baixo
#define VOLUME_AGUA_BAIXO 30 // Distância que representa nível alto

// -- Configurações do Sistema de Arquivos --
#define FORMAT_LITTLEFS_IF_FAILED true
const char* RFID_FILENAME = "/rfids.txt";
const char* WATER_LOGS_FILENAME = "/water-measurement.txt";
const char* ACCESS_LOGS_FILENAME = "/user-access.txt";

#endif // CONFIG_H