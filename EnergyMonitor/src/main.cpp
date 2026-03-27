#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "sensor.h"

TwoWire I2C_ads = TwoWire(1);

Adafruit_ADS1115 ads;

void setup() {
  Serial.begin(115200);

  delay(1000); // Aguarda o monitor serial iniciar
  I2C_ads.begin(32, 33); // sda, scl

  if (!ads.begin(0x48, &I2C_ads)) {
    Serial.println("Failed to initialize ADS1115!");
    Serial.println("Please check your wiring and I2C address.");
    while (1);
  }

  ads.setGain(GAIN_ONE);
  ads.setDataRate(RATE_ADS1115_860SPS);

  xTaskCreatePinnedToCore(
    TaskCurrentSensor,   // Function to implement the task
    "TaskCurrentSensor", // Name of the task
    2048,               // Stack size in words
    NULL,               // Task input parameter
    1,                  // Priority of the task
    NULL,               // Task handle
    0                   // Core where the task should run
  );

  Serial.println("Setup complete.");
}

void loop() {
  Serial.println("Estou vivo!");
  delay(1000);
}