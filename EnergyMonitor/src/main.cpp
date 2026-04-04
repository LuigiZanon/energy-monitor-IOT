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
#include "display.h"

#define BUTTON_PIN 13

#define SD_CS 4
#define SD_SCK 16
#define SD_MOSI 17
#define SD_MISO 18

#define DP_SDA 27
#define DP_SCL 26

SPIClass spi = SPIClass(VSPI);

TwoWire I2C_ads = TwoWire(1);
TwoWire I2C_display = TwoWire(2);

Adafruit_ADS1115 ads;
Adafruit_SSD1306 display(128, 32, &I2C_display, -1);

QueueHandle_t xQueueSensorData;
QueueHandle_t queueMQTTdata;

void setup()
{
  Serial.begin(115200);

  I2C_ads.begin(32, 33); // sda, scl

  if (!I2C_display.begin(DP_SDA, DP_SCL, 400000))
  {
    Serial.println("Failed to initialize SSD1306!");
    while (1)
      ;
  }

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println("SSD1306 allocation failed");
    while (1)
      ;
  }

  if (!ads.begin(0x48, &I2C_ads))
  {
    Serial.println("Failed to initialize ADS1115!");
    Serial.println("Please check your wiring and I2C address.");
    while (1)
      ;
  }

  ads.setGain(GAIN_ONE);
  ads.setDataRate(RATE_ADS1115_860SPS);

  spi.begin(SD_SCK, SD_MISO, SD_MOSI, -1);

  if (!SD.begin(SD_CS, spi, 4000000))
  {
    Serial.println("Falha ao montar o cartão SD. Verifique a fiação.");
    return;
  }

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE)
  {
    Serial.println("Nenhum cartão SD inserido no módulo.");
    return;
  }

  Serial.println("Cartão SD montado com sucesso na pinagem customizada!");

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
      1);

  xMutexData = xSemaphoreCreateMutex();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(BUTTON_PIN, button_isr, FALLING);
  
  xTaskCreatePinnedToCore(
    TaskDisplay, 
    "Display Task",
    4096,
    NULL,
    1,
    &xTaskDisplayHandle,
    1
  );

  Serial.println("Setup complete.");
}

void loop()
{
  vTaskDelete(NULL); // Deleta a tarefa principal, pois as tarefas de leitura e envio de dados estão rodando em paralelo
}