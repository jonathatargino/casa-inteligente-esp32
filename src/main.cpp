#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>
#include "sensor.h"

#define TRIGPIN 5
#define ECHOPIN 18
#define PINOLED 4
#define SOUND_SPEED 0.034
//talvez personalizar depois
#define VOLUMEAGUAALTO 10
#define VOLUMEAGUABAIXO 5
/*
Componentes do circuito
- 1 ESP32 30 pinos.
- 1 cabo micro USB.
- 1 protoboard.
- 1 sensor de distância HC-SR04.
- 1 leitor RFID RDM6300.
- 1 relé.
- 4 LEDs.
- 13 cabos jumper macho-fêmea.
- 4 cabos jumper macho-macho.
- 3 cabos jumper fêmea-fêmea.

Conexões do circuito
- Ligar o ESP32 em uma fonte de alimentação por meio do cabo micro USB.
- Ligar o GND do ESP32 na região de alimentação NEGATIVA da protoboard por meio de um jumper macho-fêmea.
- Posicionar os LEDs na região de componentes da protoboard com o pino negativo em paralelo.
- Ligar o GPIO13 do ESP32 no pino positivo do LED verde por meio de um jumper macho-fêmea.
- Ligar o GPIO12 do ESP32 no pino positivo do LED amarelo por meio de um jumper macho-fêmea.
- Ligar o GPIO14 do ESP32 no pino positivo do LED vermelho por meio de um jumper macho-fêmea.
- Ligar o GPIO27 do ESP32 no pino positivo do LED branco por meio de um jumper macho-fêmea.
- Ligar o pino negativo de cada LED em paralelo com o GND do ESP32 por meio de um jumper macho-macho.
- Posicionar sensor de distância na região de componentes da protoboard.
- Ligar o VIN do ESP32 na região de alimentação positiva da protoboard por meio de um jumper macho-fêmea.
- Ligar o VCC do sensor de distância em paralelo com o VIN do ESP32 por meio de um jumper macho-macho.
- Ligar o pino trigger do sensor de distância no GPIO5 do ESP32 por meio de um jumper macho-fêmea.
- Ligar o pino echo do sensor de distância no GPIO18 do ESP32 por meio de um jumper-macho-fêmea.
- Ligar o pino GND no sensor de distância em paralelo com o GND do ESP32 por meio de um jumper macho-macho.
- Ligar o pino comum do relé em paralelo com o pino positivo do LED branco por meio de um jumper macho-macho.
- Ligar o pino normalmente aberto do relé no pino 3.3V do ESP32 por meio de um jumper macho-fêmea.
- Ligar o pino GND do relé em paralelo com o GND do ESP32 por meio de um jumper macho-fêmea.
- Ligar o pino IN do relé no pino GPIO27 do ESP32 por meio de um jumper fêmea-fêmea.
- Ligar o pino VCC do relé em paralelo com o VIN do ESP32 por meio de um jumper macho-fêmea.
- Ligar o pino 5V do RDM6300 em paralelo com o VIN do ESP32 por meio de um jumper macho-fêmea.
- Ligar o pino GND do RDM6300 em paralelo com o GND do ESP32 por meio de um jumper macho-fêmea.
- Ligar o pino RX do RDM6300 no pino GPIO17 (TXD2) do ESP32 através de um jumper fêmea-fêmea.
- Ligar o pino TX do RDM6300 no pino GPIO16 (RXD2) do ESP32 através de um jumper fêmea-fêmea.
*/

#define FORMAT_LITTLEFS_IF_FAILED true

// Definições
#define RELE 14  // Corrigido para GPIO27, conforme conexões
#define BOTtoken "7344592024:AAGDK4Mu6kvV9U37gf_R8C8u32dgHDR5KEA"
#define CHAT_ID "7620344992"

// Wi-Fi
const char* ssid = "sequestro";
const char* password = "estouapto";

// Telegram
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;

// LED e Relé
const int RELEpin = RELE;
bool ledState = LOW;

// Arquivo de RFIDs
const char* filename = "/rfids.txt";

// Estado temporário
bool aguardandoCadastro = false;
bool aguardandoRemocao = false;

// Serial para RFID
#define TXD2 17
#define RXD2 16
#define RFID_BAUD 9600
HardwareSerial rfidSerial(2);

long duration;
float distanceCm;
int valor;

void useSensorAndChangeLed(){
  digitalWrite(TRIGPIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIGPIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGPIN, LOW);
  
  duration = pulseIn(ECHOPIN, HIGH);
  
  distanceCm = duration * SOUND_SPEED/2;

  Serial.println("duration");
  Serial.println(duration);

  Serial.println("distanceCm");
  Serial.println(distanceCm);
  
  if(distanceCm >= VOLUMEAGUAALTO){
      String msg = "Volume de agua baixo | distancia: " + String(distanceCm);
      // AddMessageToWaterLog(msg.c_str());
      Serial.println(msg);
      valor = 85;
  }
  if(distanceCm < VOLUMEAGUAALTO &&  distanceCm >= VOLUMEAGUABAIXO){
      String msg = "Volume de agua medio | distancia: " + String(distanceCm);
      // AddMessageToWaterLog(msg.c_str());
      Serial.println(msg);
      valor = 170;
  }
  if(distanceCm < VOLUMEAGUABAIXO){
      String msg = "Volume de agua alto  | distancia: " + String(distanceCm);
      // AddMessageToWaterLog(msg.c_str());
      Serial.println(msg);
      valor = 255;
    }
    
  ledcWrite(0, valor);
  
  delay(500);
}

void sensorTask(void* parameter) {
  while(true) {
    useSensorAndChangeLed();
    Serial.println("Rodando Sensor TASK");
    //delay de 30 segundos
    vTaskDelay(1800 / portTICK_PERIOD_MS);  
  }
}

// Inicializa LittleFS e garante que o arquivo existe
void initFS() {
  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
    Serial.println("Erro ao montar LittleFS");
    return;
  }
  if (!LittleFS.exists(filename)) {
    File file = LittleFS.open(filename, FILE_WRITE);
    if (file) file.close();
  }
}

// Adiciona RFID ao arquivo
bool addRFID(String tag) {
  if (tag.isEmpty()) return false;
  File file = LittleFS.open(filename, FILE_APPEND);
  if (!file) return false;
  file.println(tag);
  file.close();
  return true;
}

// Remove RFID do arquivo
bool removeRFID(String tagToRemove) {
  if (tagToRemove.isEmpty()) return false;
  File file = LittleFS.open(filename, "r");
  if (!file) return false;
  String temp = "";
  bool encontrado = false;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line != tagToRemove) {
      temp += line + "\n";
    } else {
      encontrado = true;
    }
  }
  file.close();
  file = LittleFS.open(filename, "w");
  if (!file) return false;
  file.print(temp);
  file.close();
  return encontrado;
}

// Verifica se o RFID está no arquivo
bool checkRFID(String tag) {
  if (tag.isEmpty()) return false;
  File file = LittleFS.open(filename, "r");
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

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;
    if (chat_id != CHAT_ID) {
      bot.sendMessage(chat_id, "Usuário não autorizado.", "");
      continue;
    }
    if (text == "/start") {
      String welcome = "Olá, " + from_name + ".\n";
      welcome += "Comandos disponíveis:\n";
      welcome += "/cadastrar - Registrar novo RFID (digite depois)\n";
      welcome += "/remover - Remover RFID (digite depois)\n";
      bot.sendMessage(chat_id, welcome, "");
    } 
    else if (text == "/cadastrar") {
      aguardandoCadastro = true;
      aguardandoRemocao = false;
      bot.sendMessage(chat_id, "Por favor, envie o número do RFID que deseja cadastrar.", "");
    } 
    else if (text == "/remover") {
      aguardandoRemocao = true;
      aguardandoCadastro = false;
      bot.sendMessage(chat_id, "Por favor, envie o número do RFID que deseja remover.", "");
    } 
    else {
      if (aguardandoCadastro) {
        if (addRFID(text)) {
          bot.sendMessage(chat_id, "RFID cadastrado com sucesso: " + text, "");
        } else {
          bot.sendMessage(chat_id, "Falha ao cadastrar o RFID.", "");
        }
        aguardandoCadastro = false;
      } 
      else if (aguardandoRemocao) {
        if (removeRFID(text)) {
          bot.sendMessage(chat_id, "RFID removido com sucesso: " + text, "");
        } else {
          bot.sendMessage(chat_id, "RFID não encontrado ou erro ao remover.", "");
        }
        aguardandoRemocao = false;
      }
    }
  }
}

// Função para ler RFID baseado no padrão do RDM6300
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
        Serial.println(); // Nova linha após a leitura completa
        return rfidTag; // Retorna a tag lida
      }
    }
  }
  return ""; // Retorna vazio se não houver tag válida
}

void setup() {
  Serial.begin(9600);
  pinMode(RELEpin, OUTPUT);
  pinMode(TRIGPIN, OUTPUT); 
  pinMode(ECHOPIN, INPUT); 
  pinMode(PINOLED, OUTPUT);
  ledcAttachPin(PINOLED, 0);
  ledcSetup(0, 500, 8);
  
  WiFi.begin(ssid, password);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  rfidSerial.begin(RFID_BAUD, SERIAL_8N1, RXD2, TXD2);
  Serial.println("\nWiFi conectado. IP: " + WiFi.localIP().toString());
  initFS();
  xTaskCreatePinnedToCore(sensorTask, "SensorTask", 4096, NULL, 1, NULL, 0);
}
// preto,marrom, laranja e azul
void loop() {
  digitalWrite(RELEpin, LOW); // Desliga o relé
  // Lê o RFID
  String rfid = readRFID();
  if (rfid != "") { // Se uma tag for lida
    Serial.println("RFID Tag: " + rfid);
    if (checkRFID(rfid)) { // Verifica se a tag está cadastrada
      Serial.println("Acesso liberado!");
       // Acende LED verde
      digitalWrite(RELEpin, HIGH); // Liga o relé
      bot.sendMessage(CHAT_ID, "Acesso liberado para RFID: " + rfid, "");
      delay(2000); // Mantém por 2 segundos
       // Apaga LED verde
      digitalWrite(RELEpin, LOW); // Desliga o relé
    } else {
      Serial.println("Acesso negado!"); // Acende LED vermelho
      bot.sendMessage(CHAT_ID, "Acesso negado para RFID: " + rfid, "");
      delay(2000); // Mantém por 2 segundos // Apaga LED vermelho
    }
  }

  // Verifica mensagens do Telegram
  if (millis() > lastTimeBotRan + botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
}