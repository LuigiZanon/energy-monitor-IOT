#include "network.h"
#include "processing.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <time.h>

// 1. Alocação das Variáveis Globais de Rede
TaskHandle_t xTaskNetworkHandle = NULL;
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// 2. Credenciais e Configurações
const char *mqtt_server = "10.228.47.30"; // IP do computador onde roda o Mosquitto
const int mqtt_port = 1883;               // TROQUE DE 5000 PARA 1883
const char *mqtt_topic = "teste/esp";
#define MAX_BATCH 10

// 3. Buffers Locais
MQTTData batchBuffer[MAX_BATCH];
int batchCount = 0;

void reconnectMQTT()
{
    Serial.print("Conectando ao MQTT...");
    String clientId = "ESPClient-" + String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str()))
    {
        Serial.println("conectado!");
    }
    else
    {
        Serial.print("falhou, rc=");
        Serial.println(mqttClient.state());
    }
}

void TaskNetwork(void *pvParameters)
{
    WiFiManager wm;
    Serial.println("Iniciando WiFiManager...");

    if (!wm.autoConnect("EnergyMonitor", ""))
    {
        Serial.println("Falha ao conectar no WiFi. Reiniciando ESP...");
        vTaskDelay(pdMS_TO_TICKS(3000));
        ESP.restart();
    }
    Serial.println("WiFi Conectado com Sucesso!");

    Serial.print("Sincronizando relógio via NTP...");

    // O padrão da indústria para IoT é salvar tudo em banco de dados como UTC (GMT 0).
    // Deixe o frontend (Grafana) fazer a conversão para o horário local.
    // Parâmetros: Fuso horário (0), Horário de verão (0), Servidores NTP
    configTime(-3 * 3600, 0, "pool.ntp.org");

    // Aguarda até o relógio interno ser atualizado (sair do ano 1970)
    time_t now = time(nullptr);
    while (now < 24 * 3600)
    {
        Serial.print(".");
        vTaskDelay(pdMS_TO_TICKS(500));
        now = time(nullptr);
    }
    Serial.println("\nRelógio interno sincronizado!");

    mqttClient.setServer(mqtt_server, mqtt_port);
    mqttClient.setBufferSize(512);

    mqttClient.setSocketTimeout(2);

    MQTTData receivedData;

    for (;;)
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("WiFi desconectado! Aguardando reconexão nativa...");
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        if (!mqttClient.connected())
        {
            reconnectMQTT();
            if (!mqttClient.connected())
            {
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue;
            }
        }
        else
        {
            mqttClient.loop();
        }

        if (xQueueReceive(queueMQTTdata, &receivedData, pdMS_TO_TICKS(50)) == pdPASS)
        {
            batchBuffer[batchCount] = receivedData;
            batchCount++;

            if (batchCount >= MAX_BATCH)
            {
                // Tamanho exato em bytes do nosso array no momento do envio
                size_t payloadSize = batchCount * sizeof(MQTTData);

                // Envia o dump de memória (cast para ponteiro de bytes)
                if (mqttClient.publish(mqtt_topic, (uint8_t *)batchBuffer, payloadSize))
                {
                    Serial.printf("Lote BINÁRIO publicado! (%d bytes)\n", payloadSize);
                }
                else
                {
                    Serial.println("Falha ao publicar lote binário (Verifique a rede).");
                }

                batchCount = 0;
            }
        }
    }
}

void IRAM_ATTR wifi_reset_isr()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(xTaskNetworkHandle, 0x01, eSetBits, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}