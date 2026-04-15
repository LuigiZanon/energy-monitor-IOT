#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads; // Instancia o ADS1115

// Multiplicador padrão para ganho de +/- 6.144V do ADS1115 (1 bit = 0.1875mV)
const float multiplier = 0.125; 

// Fator de calibração que ajustaremos depois
float calibration_factor = 1.0; 

void setup(void) {
  Serial.begin(115200);
  
  // Inicializa o I2C nos pinos padrão do ESP32 (SDA=21, SCL=22)
  Wire.begin(32, 33);

  if (!ads.begin()) {
    Serial.println("Falha ao inicializar o ADS1115!");
    while (1); // Trava aqui se der erro
  }
  
  // Configura o ganho (Opcional, o padrão é GAIN_TWOTHIRDS que lê até +/- 6.144V)
  // Como estamos em 3.3V, a onda não passará disso de qualquer forma.
  ads.setGain(GAIN_ONE); 
}

void loop(void) {
  int16_t adc0;
  int16_t max_val = -13200;
  int16_t min_val = 13200;
  
  // Tempo de amostragem de 50ms (garante pegar 3 ciclos completos em 60Hz)
  uint32_t start_time = millis();
  
  while((millis() - start_timae)) {
    adc0 = ads.readADC_SingleEnded(0);
    Serial.print(">Tensao:");
    Serial.println(adc0);
  }
  
//   Serial.print("Tensão Medida: ");
//   Serial.print(adc0);
//   Serial.println(" V"); 
}