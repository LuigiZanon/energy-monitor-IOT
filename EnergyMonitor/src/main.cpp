/*#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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

  // CONFIGURAÇÃO DE GANHO:
  // GAIN_ONE: +/- 4.096V (Ideal para leitura de 3.3V)
  // 1 bit = 0.125mV
  ads.setGain(GAIN_ONE);

  Serial.println("--- Iniciando Calibracao de Offset ---");
  Serial.println("Certifique-se de que NAO ha tensao AC no sensor.");
  delay(2000);
}

void loop() {
  
  long somaRaw = 0;
  float somaVolts = 0;
  const int numeroAmostras = 10000;

  for (int i = 0; i < numeroAmostras; i++) {
    int16_t adc0 = ads.readADC_SingleEnded(1); // Lendo pino A1
    somaRaw += adc0;
    somaVolts += ads.computeVolts(adc0);
    delay(1); // Pequeno delay para estabilidade
    Serial.println(adc0);
  }

  float offsetMedioRaw = (float)somaRaw / numeroAmostras;
  float offsetMedioVolts = somaVolts / numeroAmostras;

  Serial.print("Offset Medio (Raw): ");
  Serial.println(offsetMedioRaw);
  Serial.print("Offset Medio (Volts): ");
  Serial.print(offsetMedioVolts, 4); 
  Serial.println("Use este valor de 'Volts' no seu codigo de RMS.");
  
  delay(10000); // Repete a cada 5 segundos
}*/

//float CalculoTesao(){
/*TODO
  Descobrir como a leitura do ZMPT101B funciona
  Fazer ele se conectar com o ADS
  Executar o calculo da tensao 
  REZAR!
  //offsetRaw:12681.42
  //Meu offset: 1.6100 V
*/


//}



#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "sensor.h"
#include "display.h"

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
  
  vTaskDelete(NULL);
}