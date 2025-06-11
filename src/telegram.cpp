// telegram.cpp

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include "telegram.h"
#include "config.h"
#include "logs.h"
#include "rfid.h"

// Variáveis internas ao módulo
static WiFiClientSecure client;
static UniversalTelegramBot bot(BOT_TOKEN, client);
static int botRequestDelay = 1000;
static unsigned long lastTimeBotRan;
static bool aguardandoCadastro = false;
static bool aguardandoRemocao = false;

// Função auxiliar interna para processar novas mensagens
static void handleNewMessages(int numNewMessages) {
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
      welcome += "/cadastrar - Registrar novo RFID\n";
      welcome += "/remover - Remover RFID\n";
      welcome += "/logs_acesso - Ver logs de acesso\n";
      welcome += "/logs_medicao - Ver logs de medição\n";
      bot.sendMessage(chat_id, welcome, "");
    } 
    else if (text == "/cadastrar") {
      aguardandoCadastro = true;
      aguardandoRemocao = false;
      bot.sendMessage(chat_id, "Envie o código do RFID para cadastrar.", "");
    } 
    else if (text == "/remover") {
      aguardandoRemocao = true;
      aguardandoCadastro = false;
      bot.sendMessage(chat_id, "Envie o código do RFID para remover.", "");
    } 
    else if (text == "/logs_medicao") {
      bot.sendMessage(chat_id, readLastLogs(WATER_LOGS_FILENAME, 5));
    } 
    else if (text == "/logs_acesso") {
      bot.sendMessage(chat_id, readLastLogs(ACCESS_LOGS_FILENAME, 5));
    } 
    else {
      if (aguardandoCadastro) {
        if (addRFID(text)) {
          bot.sendMessage(chat_id, "RFID cadastrado: " + text, "");
        } else {
          bot.sendMessage(chat_id, "Falha ao cadastrar. O RFID pode já existir.", "");
        }
        aguardandoCadastro = false;
      } 
      else if (aguardandoRemocao) {
        if (removeRFID(text)) {
          bot.sendMessage(chat_id, "RFID removido: " + text, "");
        } else {
          bot.sendMessage(chat_id, "RFID não encontrado.", "");
        }
        aguardandoRemocao = false;
      }
    }
  }
}

// Função auxiliar interna para verificar mensagens periodicamente
static void handleTelegramMessages() {
  if (millis() > lastTimeBotRan + botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    if (numNewMessages > 0) {
      handleNewMessages(numNewMessages);
    }
    lastTimeBotRan = millis();
  }
}

// --- Funções Públicas ---

void initTelegram() {
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
}

void telegramTask(void* parameter) {
  Serial.println("Iniciando loop da Task de Telegram...");
  while (true) {
    handleTelegramMessages();
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}