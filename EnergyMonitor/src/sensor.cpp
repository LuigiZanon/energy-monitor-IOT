#include "sensor.h"

// Fator de conversão do Ads1115 com Gain 1 (+/- 4.096V)
#define FATOR_TENSAO_ADS 0.125

// Se o sensor for o SCT-013 100A/1V, a relação é 100 Amperes por 1 Volt
#define FATOR_CONVERSAO_SCT 100.0 

#define FATOR_CALIBRACAO_FINA 1.00 

short ciclos = 2; 

const uint32_t TEMPO_AMOSTRAGEM_US = (1000000 / 60) * ciclos;

void TaskCurrentSensor(void *pvParameters)
{
    static double offsetDC = 13200.0;

    for (;;)
    {
        uint32_t tempoInicio = micros();
        double somaQuadrados = 0;
        uint32_t totalAmostras = 0;

        while ((micros() - tempoInicio) < TEMPO_AMOSTRAGEM_US)
        {
            int16_t sample = ads.readADC_SingleEnded(0);

            offsetDC = offsetDC + ((sample - offsetDC) / 1024.0);
            double sinalFiltrado = sample - offsetDC;

            somaQuadrados += (sinalFiltrado * sinalFiltrado);
            totalAmostras++;

            taskYIELD(); 
        }

        if (totalAmostras > 0)
        {
            double rmsADC = sqrt(somaQuadrados / totalAmostras);
            float correnteRMS = (rmsADC * FATOR_TENSAO_ADS) / 1000.0;

            float correnteTeorica = correnteRMS * FATOR_CONVERSAO_SCT;
            
            float correnteReal = correnteTeorica * FATOR_CALIBRACAO_FINA;

            if (correnteReal < 0.05) {
                correnteReal = 0.00;
            }

            Serial.print(">Corrente:");
            Serial.println(correnteReal);
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}