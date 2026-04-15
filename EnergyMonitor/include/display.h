#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>

extern Adafruit_SSD1306 display;

enum DisplayMode
{
    SHOW_V1,
    SHOW_A1,
    SHOW_POWER,
    MAX_MODES
};

typedef struct 
{
    float v1, a1, watts;
} DisplayData;

extern DisplayData liveData;
extern volatile DisplayMode currentMode;
extern SemaphoreHandle_t xMutexData;
extern TaskHandle_t xTaskDisplayHandle;

void TaskDisplay(void *pvParameters);

void IRAM_ATTR button_isr();

#endif // DISPLAY_H