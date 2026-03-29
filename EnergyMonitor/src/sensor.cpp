#include "sensor.h"

// Fator de conversão do Ads1115 com Gain 1 (+/- 4.096V)
#define FATOR_TENSAO_ADS 0.125

// Se o sensor for o SCT-013 100A/1V, a relação é 100 Amperes por 1 Volt
#define FATOR_CONVERSAO_SCT 100.0 
#define FATOR_CONVERSAO_ZMPT 610.0

#define FATOR_CALIBRACAO_FINA 1.00 

short ciclos = 2; 

const uint32_t TEMPO_AMOSTRAGEM_US = (1000000 / 60) * ciclos;


void TaskCurrentSensor(void *pvParameters)
{
    static double offsetDC_SCT = 13200.0;
    static double offset_ZMPT = 12681.42;

    for (;;)
    {
        uint32_t tempoInicio = micros();
        double somaQuadrados_SCT = 0;
        uint32_t totalAmostras = 0;
        double somaQuadrados_ZMPT = 0;

        while ((micros() - tempoInicio) < TEMPO_AMOSTRAGEM_US)
        {
            int16_t sample_SCT = ads.readADC_SingleEnded(0);
            int16_t sample_ZMPT = ads.readADC_SingleEnded(1);

            offsetDC_SCT = offsetDC_SCT + ((sample_SCT - offsetDC_SCT) / 1024.0);
            offset_ZMPT = offset_ZMPT + ((sample_ZMPT - offset_ZMPT) / 1024.0);


            double sinalFiltrado_SCT = sample_SCT - offsetDC_SCT;
            double sinalFiltrado_ZMPT = sample_ZMPT - offset_ZMPT;

            somaQuadrados_SCT += (sinalFiltrado_SCT * sinalFiltrado_SCT);
            somaQuadrados_ZMPT += (sinalFiltrado_ZMPT * sinalFiltrado_ZMPT);
            totalAmostras++;

            taskYIELD(); 
        }

        if (totalAmostras > 0)
        {
            double rmsADC = sqrt(somaQuadrados_SCT / totalAmostras);
            float correnteRMS = (rmsADC * FATOR_TENSAO_ADS) / 1000.0;
            float correnteTeorica = correnteRMS * FATOR_CONVERSAO_SCT;
            float correnteReal = correnteTeorica * FATOR_CALIBRACAO_FINA;

            double rmsADC_ZMPT = sqrt(somaQuadrados_ZMPT / totalAmostras);
            float vRMS_ZMPT_Baixa = (rmsADC_ZMPT * FATOR_TENSAO_ADS) / 1000.0;
            float tensaoReal = vRMS_ZMPT_Baixa * FATOR_CONVERSAO_ZMPT;

            if (correnteReal < 0.05) {
                correnteReal = 0.00;
            }

            //Serial.print(">Corrente:");
            //Serial.println(correnteReal);
            Serial.print(">Volts");
            Serial.println(tensaoReal);
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}