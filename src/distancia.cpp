// distancia.cpp

#include <Arduino.h>
#include "distancia.h"
#include "config.h"
#include "logs.h"

// Função auxiliar interna ao módulo
static void measureDistanceAndControlLed() {
  digitalWrite(DISTANCIA_TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(DISTANCIA_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(DISTANCIA_TRIG_PIN, LOW);

  long duration = pulseIn(DISTANCIA_ECHO_PIN, HIGH);
  float distanceCm = duration * SOUND_SPEED / 2;
  int ledValue = 0;
  String msg;

  if (distanceCm >= VOLUME_AGUA_ALTO) {
    msg = "Volume de agua baixo | distancia: " + String(distanceCm);
    ledValue = 20;
  } else if (distanceCm < VOLUME_AGUA_ALTO && distanceCm >= VOLUME_AGUA_BAIXO) {
    msg = "Volume de agua medio | distancia: " + String(distanceCm);
    ledValue = 90;
  } else {
    msg = "Volume de agua alto  | distancia: " + String(distanceCm);
    ledValue = 255;
  }

  addLog(WATER_LOGS_FILENAME, msg);
  ledcWrite(0, ledValue);
}

void initDistancia() {
  pinMode(DISTANCIA_TRIG_PIN, OUTPUT);
  pinMode(DISTANCIA_ECHO_PIN, INPUT);
  pinMode(LED_NIVEL_PIN, OUTPUT);
  ledcAttachPin(LED_NIVEL_PIN, 0);
  ledcSetup(0, 500, 8); // Canal 0, Frequência de 500Hz, Resolução de 8 bits
}

void sensorTask(void* parameter) {
  while (true) {
    measureDistanceAndControlLed();
    Serial.println("Rodando Sensor Task");
    vTaskDelay(18000 / portTICK_PERIOD_MS); // Executa a cada 18 segundos
  }
}