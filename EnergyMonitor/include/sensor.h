#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <Adafruit_ADS1X15.h>

//  Declaracao do ads, extern pois e definido em main.cpp
extern Adafruit_ADS1115 ads;

extern QueueHandle_t xQueueSensorData;

typedef struct {
    float sct_1;
    float sct_2;
    float zmpt_1;
    float zmpt_2;
    uint16_t samples;
} SensorData;

void TaskCurrentSensor(void *pvParameters);

void TaskSensorsRawData(void *pvParameters);

#endif