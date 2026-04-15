#ifndef PROCESSING_H
#define PROCESSING_H

#include <mat.h>
#include <SD.h>

#define FATOR_TENSAO_ADS 0.125

#define FATOR_CONVERSAO_SCT 100.0
#define FATOR_CONVERSAO_ZMPT 800.21

#define FATOR_CALIBRACAO_FINA 1.00

typedef struct {
    uint32_t timestamp;
    float rms_sct1;
    float rms_sct2;
    float rms_zmpt1;
    float rms_zmpt2;
} MQTTData;

extern QueueHandle_t queueMQTTdata;

void TaskProcessSensorData(void *pvParameters);

#endif