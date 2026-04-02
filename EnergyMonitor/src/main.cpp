#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "sensor.h"
#include "processing.h"

#define SD_CS   15
#define SD_SCK  17
#define SD_MOSI 5
#define SD_MISO 18

SPIClass spi = SPIClass(VSPI);

TwoWire I2C_ads = TwoWire(1);

Adafruit_ADS1115 ads;

QueueHandle_t xQueueSensorData;
QueueHandle_t queueMQTTdata;

void setup()
{
  Serial.begin(115200);

  delay(1000);           // Aguarda o monitor serial iniciar
  I2C_ads.begin(32, 33); // sda, scl

  if (!ads.begin(0x48, &I2C_ads))
  {
    Serial.println("Failed to initialize ADS1115!");
    Serial.println("Please check your wiring and I2C address.");
    while (1)
      ;
  }

  pinMode(19, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);

  ads.setGain(GAIN_ONE);
  ads.setDataRate(RATE_ADS1115_860SPS);

  if (!SD.begin(5, spi, 4000000)) { 
    Serial.println("Falha ao montar o cartão SD. Verifique a fiação.");
    return;
  }

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("Nenhum cartão SD inserido no módulo.");
    return;
  }

  Serial.println("Cartão SD montado com sucesso na pinagem customizada!");

  /*xTaskCreatePinnedToCore(
    TaskCurrentSensor,   // Function to implement the task
    "TaskCurrentSensor", // Name of the task
    2048,               // Stack size in words
    NULL,               // Task input parameter
    1,                  // Priority of the task
    NULL,               // Task handle
    0                   // Core where the task should run
  );*/

  xQueueSensorData = xQueueCreate(10, sizeof(SensorData));
  queueMQTTdata = xQueueCreate(60, sizeof(MQTTData));

  uint32_t send_rate_ms = 1000; // Taxa de envio em ms

  xTaskCreatePinnedToCore(
      TaskSensorsRawData,   // Function to implement the task
      "TaskSensorsRawData", // Name of the task
      4096,                 // Stack size in words
      (void *)send_rate_ms, // Task input parameter
      5,                    // Priority of the task
      NULL,                 // Task handle
      0                     // Core where the task should run
  );

  xTaskCreatePinnedToCore(
    TaskProcessSensorData,
    "TaskProcessSensorData",
    4096,
    NULL,
    4,
    NULL,
    1
  );

  Serial.println("Setup complete.");
}

void loop()
{
  vTaskDelete(NULL); // Deleta a tarefa principal, pois as tarefas de leitura e envio de dados estão rodando em paralelo
}