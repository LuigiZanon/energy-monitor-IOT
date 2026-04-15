#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>

extern TaskHandle_t xTaskNetworkHandle;

void TaskNetwork(void *pvParameters);
void IRAM_ATTR wifi_reset_isr();

#endif // NETWORK_H