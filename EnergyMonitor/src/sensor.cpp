#include "sensor.h"

// possivelmente necessaria alteracao para offset dinamico
#define OFFSET 13200.0

short ciclos = 2;

const uint32_t TEMPO_AMOSTRAGEM_US = (1000000 / 60) * ciclos;

/*void TaskCurrentSensor(void *pvParameters)
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

            Serial.print(">Amostra_bruta:");
            Serial.println(sinalFiltrado);

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

            if (correnteReal < 0.05)
            {
                correnteReal = 0.00;
            }

            Serial.print(">Corrente:");
            Serial.println(correnteReal);
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}*/

void TaskSensorsRawData(void *pvParameters)
{
    // taxa de envio em ms vindo como parametro da tarefa
    uint32_t send_rate = (uint32_t)pvParameters;

    // periodica de leitura de 5 ms para manter acoplamento com a taxa desejada
    const uint32_t loop_period_ms = 5;
    const TickType_t xFrequency = pdMS_TO_TICKS(loop_period_ms);

    // numero de amostras por pacote de envio
    uint32_t target_samples = send_rate / loop_period_ms;
    if (target_samples == 0)
        target_samples = 1; // garante ao menos 1 amostra

    // estrutura para acumular os quadrados de cada sensor
    SensorData rawDataBatch = {0, 0, 0, 0, 0};
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        // leitura dos 4 canais do ADS1115
        int16_t sampleSCT1 = ads.readADC_SingleEnded(0);
        int16_t sampleSCT2 = ads.readADC_SingleEnded(2);
        int16_t sampleZMPT1 = ads.readADC_SingleEnded(1);
        int16_t sampleZMPT2 = ads.readADC_SingleEnded(3);

        // subtrai offset DC para centralizar o sinal em torno de 0
        float sct1 = (float)sampleSCT1 - OFFSET;
        float sct2 = (float)sampleSCT2 - OFFSET;
        float zmpt1 = (float)sampleZMPT1 - OFFSET;
        float zmpt2 = (float)sampleZMPT2 - OFFSET;

        // acumula o valor ao quadrado para calculo de RMS posterior
        rawDataBatch.sct_1 += (sct1 * sct1);
        rawDataBatch.sct_2 += (sct2 * sct2);
        rawDataBatch.zmpt_1 += (zmpt1 * zmpt1);
        rawDataBatch.zmpt_2 += (zmpt2 * zmpt2);
        rawDataBatch.samples++;

        // ao atingir numero de amostras definido, envia ao consumidor e zera acumuladores
        if (rawDataBatch.samples >= target_samples)
        {
            xQueueSend(xQueueSensorData, &rawDataBatch, 0);

            rawDataBatch.sct_1 = 0;
            rawDataBatch.sct_2 = 0;
            rawDataBatch.zmpt_1 = 0;
            rawDataBatch.zmpt_2 = 0;
            rawDataBatch.samples = 0;
        }

        // controla periodicidade da tarefa, evitando loop apertado
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}