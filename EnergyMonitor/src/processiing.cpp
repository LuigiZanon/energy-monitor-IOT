#include "processing.h"
#include "sensor.h"
#include "display.h"

void TaskProcessSensorData(void *pvParameters)
{

    SensorData receivedData;
    MQTTData mqttData;

    for (;;)
    {
        if (xQueueReceive(xQueueSensorData, &receivedData, portMAX_DELAY) == pdPASS)
        {
            if (receivedData.samples > 0)
            {
                float rmsSCT1_ADC = sqrt(receivedData.sct_1 / receivedData.samples);
                float rmsSCT2_ADC = sqrt(receivedData.sct_2 / receivedData.samples);
                float rmsZMPT1_ADC = sqrt(receivedData.zmpt_1 / receivedData.samples);
                float rmsZMPT2_ADC = sqrt(receivedData.zmpt_2 / receivedData.samples);

                float currentRMS_SCT1 = ((rmsSCT1_ADC * FATOR_TENSAO_ADS) / 1000.0) * FATOR_CONVERSAO_SCT;
                float currentRMS_SCT2 = ((rmsSCT2_ADC * FATOR_TENSAO_ADS) / 1000.0) * FATOR_CONVERSAO_SCT;

                float tensionRMS_ZMPT1 = ((rmsZMPT1_ADC * FATOR_TENSAO_ADS) / 1000.0) * FATOR_CONVERSAO_ZMPT;
                float tensionRMS_ZMPT2 = ((rmsZMPT2_ADC * FATOR_TENSAO_ADS) / 1000.0) * FATOR_CONVERSAO_ZMPT;

                mqttData.rms_sct1 = currentRMS_SCT1;
                mqttData.rms_sct2 = currentRMS_SCT2;

                mqttData.rms_zmpt1 = tensionRMS_ZMPT1;   // ! apenas para testes
                mqttData.rms_zmpt2 = tensionRMS_ZMPT2;   // ! apenas para testes

                // Serial.print(">Corrente:");
                // Serial.println(mqttData.rms_sct1);

                if (xSemaphoreTake(xMutexData, portMAX_DELAY) == pdTRUE)
                {
                    liveData.v1 = mqttData.rms_zmpt1; // zmpt1
                    liveData.a1 = mqttData.rms_sct1;
                    liveData.v2 = mqttData.rms_zmpt2; // zmpt2 
                    liveData.a2 = mqttData.rms_sct2;
                    liveData.watts = mqttData.rms_sct1 * mqttData.rms_zmpt1; // Cálculo simplificado
                    xSemaphoreGive(xMutexData);
                }
            }
            else
            {
                mqttData.rms_sct1 = 0;
                mqttData.rms_sct2 = 0;

                mqttData.rms_zmpt1 = 0;
                mqttData.rms_zmpt2 = 0;
            }

            // mqttData.timestamp = millis();

            time_t tempo_atual;
            time(&tempo_atual);
            mqttData.timestamp = (uint32_t)tempo_atual;

            char csvLine[64];

            snprintf(csvLine, sizeof(csvLine), "%lu,%.2f,%.2f,%.2f,%.2f\n",
                     mqttData.timestamp,
                     mqttData.rms_sct1, mqttData.rms_sct2,
                     mqttData.rms_zmpt1, mqttData.rms_zmpt2);

            File file = SD.open("/datalog.csv", FILE_APPEND);

            if (file)
            {
                file.print(csvLine);
                file.close();
            }
            else
            {
                Serial.print("Erro ao abrir o arquivo");
            }

            if (queueMQTTdata != NULL)
            {
                xQueueSend(queueMQTTdata, &mqttData, 0);
            }
        }
    }
}