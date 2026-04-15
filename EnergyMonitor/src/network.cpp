#include "network.h"
#include "processing.h" 
#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>

// 1. Alocação das Variáveis Globais de Rede
TaskHandle_t xTaskNetworkHandle = NULL;
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// 2. Credenciais e Configurações
const char* mqtt_server = "10.228.47.32";
const int mqtt_port = 5000;
const char* mqtt_topic = "teste/esp";

#define MAX_BATCH 10

// 3. Buffers Locais
MQTTData batchBuffer[MAX_BATCH];
int batchCount = 0;


void reconnectMQTT() {
    Serial.print("Conectando ao MQTT...");
    String clientId = "ESPClient-" + String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str())) {
        Serial.println("conectado!");
    } else {
        Serial.print("falhou, rc=");
        Serial.println(mqttClient.state());
    }
}

void TaskNetwork(void *pvParameters) {
    WiFiManager wm;
    Serial.println("Iniciando WiFiManager...");
    
    if(!wm.autoConnect("EnergyMonitor", "")) {
        Serial.println("Falha ao conectar no WiFi. Reiniciando ESP...");
        vTaskDelay(pdMS_TO_TICKS(3000));
        ESP.restart();
    }
    Serial.println("WiFi Conectado com Sucesso!");

    mqttClient.setServer(mqtt_server, mqtt_port);
    mqttClient.setBufferSize(512); 

    MQTTData receivedData;

    for (;;) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi desconectado! Aguardando reconexão nativa...");
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        if (!mqttClient.connected()) {
            reconnectMQTT();
            if (!mqttClient.connected()) {
                vTaskDelay(pdMS_TO_TICKS(5000)); 
                continue; 
            }
        } else {
            mqttClient.loop(); 
        }

        if (xQueueReceive(queueMQTTdata, &receivedData, pdMS_TO_TICKS(50)) == pdPASS) {
            batchBuffer[batchCount] = receivedData;
            batchCount++;

            if (batchCount >= MAX_BATCH) {
                if (mqttClient.publish(mqtt_topic, (uint8_t*)batchBuffer, batchCount * sizeof(MQTTData))) {
                    Serial.println("Lote de dados publicado via MQTT!");
                } else {
                    Serial.println("Falha ao publicar MQTT (Tamanho do buffer?).");
                }
                batchCount = 0; 
            }
        }
    }
}

void IRAM_ATTR wifi_reset_isr() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(xTaskNetworkHandle, 0x01, eSetBits, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}