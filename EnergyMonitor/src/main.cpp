#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


void taskMedirCorrente(void *pvParameters);
void taskDisplay(void *pvParameters);

// --- CONFIGURAÇÃO DOS DOIS BARRAMENTOS I2C ---
TwoWire I2C_ADS = TwoWire(1);

// --- CONFIGURAÇÃO DA FILA PARA COMUNICAÇÃO ENTRE TASKS ---
QueueHandle_t filaCorrente;

// --- CONFIGURAÇÕES DO DISPLAY OLED ---
#define LARGURA_TELA 128
#define ALTURA_TELA 32
#define PINO_RESET -1

// Display usa Wire (barramento padrão)
Adafruit_SSD1306 display(LARGURA_TELA, ALTURA_TELA, &Wire, PINO_RESET);

// --- CONFIGURAÇÕES DO SENSOR E ADC ---
// ADS usa I2C_ADS (segundo barramento)
Adafruit_ADS1115 ads;
const float RESISTOR_BURDEN = 22.0; 
const float RELACAO_SENSOR = 2000.0;
const float FATOR_TENSAO_ADS = 0.125; // mV por bit no GAIN_ONE
const float ICAL = 105.0;

#define TEMPO_AMOSTRAGEM 200 // Tempo de amostragem em milissegundos

void setup() {
  Serial.begin(115200);
  delay(500); // Aguarda estabilização do Serial

  // Configuração dos barramentos I2C
  Wire.begin(26, 25);       // I2C0 (Display): SDA = 26, SCL = 25
  I2C_ADS.begin(33, 32);    // I2C1 (ADS1115): SDA = 33, SCL = 32

  // Inicializa o ADS1115
  if (!ads.begin(0x48, &I2C_ADS)) {
    Serial.println("Falha ao iniciar o ADS1115!");
    while (1); // Trava aqui se der erro de hardware
  }
  Serial.println("ADS1115 inicializado com sucesso!");
  
  // Inicializa o Display OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Falha ao iniciar o display!");
    while (1);
  }
  Serial.println("Display OLED inicializado com sucesso!");
  display.clearDisplay();
  display.setTextColor(WHITE);

  // Cria a fila DEPOIS dos hardwares estarem prontos
  filaCorrente = xQueueCreate(5, sizeof(float));
  if (filaCorrente == NULL) {
    Serial.println("Falha ao criar a fila!");
    while (1);
  }
  Serial.println("Fila criada com sucesso!");

  // Cria as tasks POR ÚLTIMO
  xTaskCreatePinnedToCore(taskMedirCorrente, "Medicao", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(taskDisplay, "Display", 4096, NULL, 1, NULL, 1);
  Serial.println("Tasks iniciadas com sucesso!");
}

void loop() {
  // put your main code here, to run repeatedly:
}


void taskMedirCorrente(void *pvParameters) {
  // A variável offsetI precisa ser inicializada fora do loop da task
  double offsetI = 13200.0; 

  for (;;) { // Loop infinito obrigatório em Tasks FreeRTOS
    uint32_t tempoInicio = millis();
    double sumI = 0;
    int amostras = 0;

    // Seu loop de amostragem original
    while ((millis() - tempoInicio) < TEMPO_AMOSTRAGEM) {
      int16_t sampleI = ads.readADC_SingleEnded(0);

      offsetI = offsetI + ((sampleI - offsetI) / 1024.0);
      double filteredI = sampleI - offsetI;

      sumI += (filteredI * filteredI);
      amostras++;
      
      // taskYIELD() permite que outras tasks rodem rapidamente se necessário, 
      // sem pausar a task atual de forma agressiva como o vTaskDelay faria.
      taskYIELD(); 
    }

    double irms_adc = sqrt(sumI / amostras);
    float tensao_rms = (irms_adc * FATOR_TENSAO_ADS) / 1000.0;
    float correnteReal = tensao_rms * ICAL;

    if (correnteReal < 0.05) {
      correnteReal = 0.00;
    }

    // Envia o valor calculado para a Fila
    // portMAX_DELAY faz a task esperar caso a fila esteja cheia
    xQueueSend(filaCorrente, &correnteReal, portMAX_DELAY);

    // Pausa a task por 1 segundo antes de fazer a próxima medição (ajuste como quiser)
    vTaskDelay(pdMS_TO_TICKS(1000)); 
  }
}

void taskDisplay(void *pvParameters) {
  float correnteRecebida;

  for (;;) {
    // Fica bloqueado aqui até que chegue um dado na fila
    if (xQueueReceive(filaCorrente, &correnteRecebida, portMAX_DELAY) == pdPASS) {
      
      // Exemplo de como você mostraria isso (substitua pelo código do seu display)
      Serial.print("Corrente RMS: ");
      Serial.print(correnteRecebida);
      Serial.println(" A\n");
      
      display.clearDisplay(); 
  
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.print("Corrente (SCT-013):");
      
      display.setTextSize(3); 
      display.setCursor(0, 10);
      display.print(correnteRecebida, 2); 
      
      display.setTextSize(2);
      display.print(" A");
      
      display.display(); 
    }
  }
}