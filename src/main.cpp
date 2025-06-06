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
#define VOLUMEAGUAALTO 70
#define VOLUMEAGUABAIXO 30
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

void checkLittleFSSpace() {
  size_t totalBytes = LittleFS.totalBytes();
  size_t usedBytes = LittleFS.usedBytes();
  Serial.println("Espaço total em LittleFS: " + String(totalBytes) + " bytes");
  Serial.println("Espaço usado em LittleFS: " + String(usedBytes) + " bytes");
  Serial.println("Espaço livre em LittleFS: " + String(totalBytes - usedBytes) + " bytes");
}

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
const char* water_measurement_logs_filename = "/water-measurement.txt";
const char* user_access_logs_filename = "/user-access.txt";

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

bool addLogs(String filename, String log) {
  if (log.isEmpty()) return false;
  File file = LittleFS.open(filename, FILE_APPEND);
  if (!file) return false;

  unsigned long currentMillis = millis();

  // Calculate days, hours, minutes, and seconds from milliseconds
  unsigned long seconds = currentMillis / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  unsigned long days = hours / 24;

  // Get the remainder for each unit
  currentMillis %= 1000;
  seconds %= 60;
  minutes %= 60;
  hours %= 24;

  // Format the timestamp string
  String timestamp = String(days) + "-" +
                     (hours < 10 ? "0" : "") + String(hours) + ":" +
                     (minutes < 10 ? "0" : "") + String(minutes) + ":" +
                     (seconds < 10 ? "0" : "") + String(seconds);


  // Combine the timestamp with your message
  String logEntry = "[" + timestamp + "] " + log;
  if (file.println(logEntry)) {
  Serial.println("Log gravado com sucesso: " + logEntry);
  } else {
    Serial.println("Erro ao gravar log no arquivo: " + filename);
  }
  file.close();
  return true;
}

void useSensorAndChangeLed(){
  digitalWrite(TRIGPIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIGPIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGPIN, LOW);
  
  duration = pulseIn(ECHOPIN, HIGH);
  
  distanceCm = duration * SOUND_SPEED/2;
  
  if(distanceCm >= VOLUMEAGUAALTO){
      String msg = "Volume de agua baixo | distancia: " + String(distanceCm);
      addLogs(water_measurement_logs_filename, msg.c_str());
      valor = 20;
  }
  if(distanceCm < VOLUMEAGUAALTO &&  distanceCm >= VOLUMEAGUABAIXO){
      String msg = "Volume de agua medio | distancia: " + String(distanceCm);
      addLogs(water_measurement_logs_filename, msg.c_str());
      valor = 90;
  }
  if(distanceCm < VOLUMEAGUABAIXO){
      String msg = "Volume de agua alto  | distancia: " + String(distanceCm);
      addLogs(water_measurement_logs_filename, msg.c_str());
      valor = 255;
    }
    
  ledcWrite(0, valor);
  
  delay(500);
}

void sensorTask(void* parameter) {
  while(true) {
    useSensorAndChangeLed();
    Serial.println("Rodando Sensor TASK");
    vTaskDelay(18000 / portTICK_PERIOD_MS);  
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

  if (!LittleFS.exists(water_measurement_logs_filename)) {
    File file = LittleFS.open(water_measurement_logs_filename, FILE_WRITE);
    if (file) file.close();
  }

  if (!LittleFS.exists(user_access_logs_filename)) {
    File file = LittleFS.open(user_access_logs_filename, FILE_WRITE);
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


String readLastLogs(String filename, int quantity) {
  File file = LittleFS.open(filename, "r");
  if (!file) {
    Serial.println("Erro ao abrir o arquivo de log: " + filename);
    return "Erro ao abrir o arquivo de log.";
  }

  // Obtém o tamanho do arquivo
  size_t fileSize = file.size();
  if (fileSize == 0) {
    file.close();
    Serial.println("Arquivo vazio: " + filename);
    return "Nenhum log encontrado.";
  }

  // Buffer para armazenar as últimas linhas
  String* lines = new String[quantity];
  int lineCount = 0;
  String currentLine = "";

  // Posiciona o cursor no final do arquivo
  file.seek(fileSize);

  // Lê o arquivo de trás para frente, byte a byte
  long pos = fileSize - 1; // Começa do último byte
  while (pos >= 0 && lineCount < quantity) {
    file.seek(pos); // Move o cursor para a posição atual
    char c = file.read(); // Lê um byte

    // Ignora o último '\n' do arquivo, se houver
    if (pos == fileSize - 1 && c == '\n') {
      pos--;
      continue;
    }

    if (c == '\n' && currentLine.length() > 0) {
      // Encontrou uma linha completa, armazena no buffer
      lines[lineCount] = currentLine;
      lineCount++;
      currentLine = ""; // Reseta a linha atual
    } else {
      // Adiciona o caractere à linha atual (em ordem reversa)
      currentLine = c + currentLine;
    }

    pos--; // Move para o byte anterior
  }

  // Adiciona a última linha (se não terminou com '\n')
  if (currentLine.length() > 0 && lineCount < quantity) {
    lines[lineCount] = currentLine;
    lineCount++;
  }

  file.close();

  if (lineCount == 0) {
    delete[] lines;
    Serial.println("Nenhum log encontrado em: " + filename);
    return "Nenhum log encontrado.";
  }

  // Constrói o resultado na ordem correta (do mais recente ao mais antigo)
  String result = "";
  for (int i = lineCount - 1; i >= 0; i--) {
    result += lines[i] + "\n";
  }

  delete[] lines; // Libera a memória
  return result;
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
      welcome += "/logs_acesso - Ver quem tentou acessar a tranca de segurança\n";
      welcome += "/logs_medicao - Ver as últimas medições\n";
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
    else if (text == "/logs_medicao") {
      bot.sendMessage(chat_id, readLastLogs(water_measurement_logs_filename, 5));
    }
    else if (text == "/logs_acesso") {
      bot.sendMessage(chat_id, readLastLogs(user_access_logs_filename, 5));
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
  checkLittleFSSpace();
}
// preto,marrom, laranja e azul
void loop() {
  digitalWrite(RELEpin, LOW); // Desliga o relé
  // Lê o RFID
  String rfid = readRFID();
  if (rfid != "") { // Se uma tag for lida
    if (checkRFID(rfid)) { // Verifica se a tag está cadastrada
       // Acende LED verde
      digitalWrite(RELEpin, HIGH); // Liga o relé
      addLogs(user_access_logs_filename, "Acesso liberado para RFID: " + rfid);
      delay(2000); // Mantém por 2 segundos
       // Apaga LED verde
      digitalWrite(RELEpin, LOW); // Desliga o relé
    } else {
      addLogs(user_access_logs_filename, "Acesso negado para RFID: " + rfid);
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