#include "EmonLib.h"

EnergyMonitor SCT013;

int pinSCT = 34;

void setup() {
  Serial.begin(115200);

  SCT013.current(pinSCT, 0.026);
  analogReadResolution(12);
}

void loop() {
  double irms = SCT013.calcIrms(4096);

  uint32_t tensao_mV = analogReadMilliVolts(pinSCT);
  float tensao_corrigida = tensao_mV / 1000.0;

  double corrente_real = irms * 2000;

  Serial.print("Tensão(V):");
  Serial.print(tensao_corrigida, 3);
  
  // Serial.print(",");
  
  Serial.print("Corrente(A):");
  Serial.print(corrente_real, 3);

  int max_mV = 0;
  int min_mV = 4000; 

  // Define o tempo de início e a janela de amostragem
  uint32_t tempo_inicio = millis();
  uint32_t janela_amostragem = 50; // 50 milissegundos (captura ~3 ondas de 60Hz)

  // O ESP32 vai ficar preso neste loop "metralhando" leituras durante 50ms
  while ((millis() - tempo_inicio) < janela_amostragem) {
    int leitura_atual = analogReadMilliVolts(pinSCT);

    // Atualiza o valor máximo se a leitura atual for maior
    if (leitura_atual > max_mV) {
      max_mV = leitura_atual;
    }
    // Atualiza o valor mínimo se a leitura atual for menor
    if (leitura_atual < min_mV) {
      min_mV = leitura_atual;
    }
  }

  // Terminou os 50ms? Calcula a diferença entre o topo e o fundo da onda
  float tensao_pico_a_pico = (max_mV - min_mV) / 1000.0; // Divide por 1000 para converter para Volts

  // --- Formato para o Serial Plotter ---
  Serial.print("Vpp_Medio(V):");
  Serial.println(tensao_pico_a_pico, 4);
}