#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <Adafruit_ADS1X15.h>

//  Declaracao do ads, extern pois e definido em main.cpp
extern Adafruit_ADS1115 ads;

void TaskCurrentSensor(void *pvParameters);

#endif