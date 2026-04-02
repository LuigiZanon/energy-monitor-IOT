#include <Arduino.h>

// Definimos o pino do LED
const int LED_PIN = 13;

void setup() {
    // Configura o pino como saída
    pinMode(LED_PIN, OUTPUT);
}

void loop() {
    // Liga o LED
    digitalWrite(LED_PIN, HIGH);
    
    // Opcional: Se quiser que ele pisque, descomente as linhas abaixo
    /*
    delay(1000);
    digitalWrite(LED_PIN, LOW);
    delay(1000);
    */
}