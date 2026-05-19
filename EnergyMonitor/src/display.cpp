#include "display.h"

DisplayData liveData;
volatile DisplayMode currentMode = SHOW_A1;
SemaphoreHandle_t xMutexData = NULL;
TaskHandle_t xTaskDisplayHandle = NULL;

void TaskDisplay(void *pvParameters)
{
    uint32_t notificationValue;

    for (;;)
    {
        if (xTaskNotifyWait(0, 0xFFFFFFFF, &notificationValue, pdMS_TO_TICKS(500)) == pdPASS)
        {
            currentMode = (DisplayMode)((currentMode + 1) % MAX_MODES);
            Serial.printf("Botao apertado! Novo modo: %d\n", currentMode);
        }

        DisplayData temp;

        if (xSemaphoreTake(xMutexData, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            temp = liveData;
            xSemaphoreGive(xMutexData);
        }

        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);

        switch (currentMode)
        {
            case SHOW_V1:
                display.println("TENSAO FASE 1");
                display.setTextSize(2);
                display.printf("%.1f V", temp.v1);
                break;
            case SHOW_A1:
                display.println("CORRENTE FASE 1");
                display.setTextSize(2);
                display.printf("%.2f A", temp.a1);
                break;
            case SHOW_POWER:
                display.println("POTENCIA ATIVA");
                display.setTextSize(2);
                display.printf("%.0f W", temp.watts);
                break;
            case SHOW_V2:
                display.println("TENSAO FASE 2");
                display.setTextSize(2);
                display.printf("%.1f V", temp.v2); // Substitua por temp.v2 quando disponível
                break;
            case SHOW_A2:
                display.println("CORRENTE FASE 2");
                display.setTextSize(2);
                display.printf("%.2f A", temp.a2);
                break;
        }

        display.display();
    }
}

void IRAM_ATTR button_isr()
{
    static uint32_t last_interrupt_time = 0;
    uint32_t interrupt_time = xTaskGetTickCountFromISR();

    // Se passou mais de 200ms desde o ÚLTIMO clique válido
    if (interrupt_time - last_interrupt_time > pdMS_TO_TICKS(200))
    {
        // Envia a notificação para trocar a tela
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xTaskNotifyFromISR(xTaskDisplayHandle, 0x01, eSetBits, &xHigherPriorityTaskWoken);
        
        // MOVER PARA CÁ: Só atualiza o tempo se foi um clique válido!
        last_interrupt_time = interrupt_time; 
        
        // Força a troca de contexto se a Task do Display tiver prioridade maior
        if (xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }
    }
}