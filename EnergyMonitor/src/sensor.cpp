#include "sensor.h"

// Variáveis globais para guardar o centro de cada sensor
float offset_SCT1 = 0;
float offset_SCT2 = 0;

void calibrarOffsets() {
    Serial.println("Calibrando Offsets dos SCTs...");
    long soma_SCT1 = 0;
    long soma_SCT2 = 0;
    int amostras_calibracao = 0;
    
    uint32_t tempoInicio = millis();
    
    // Lê os canais o mais rápido possível durante 1 segundo
    while (millis() - tempoInicio < 1000) {
        soma_SCT1 += ads.readADC_SingleEnded(0);
        soma_SCT2 += ads.readADC_SingleEnded(2);
        amostras_calibracao++;
    }
    
    // Calcula a média exata (o novo "Zero") de cada sensor
    offset_SCT1 = (float)soma_SCT1 / amostras_calibracao;
    offset_SCT2 = (float)soma_SCT2 / amostras_calibracao;
    
    Serial.printf("Offset SCT1: %.2f | Offset SCT2: %.2f\n", offset_SCT1, offset_SCT2);
}

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
        float sct1 = (float)sampleSCT1 - offset_SCT1;
        float sct2 = (float)sampleSCT2 - offset_SCT2;
        float zmpt1 = (float)sampleZMPT1 - OFFSET_ZMPT;
        float zmpt2 = (float)sampleZMPT2 - OFFSET_ZMPT;

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