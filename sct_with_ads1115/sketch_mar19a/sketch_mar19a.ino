#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- CONFIGURAÇÃO DOS DOIS BARRAMENTOS I2C ---
TwoWire I2C_ADS = TwoWire(1);

// --- CONFIGURAÇÕES DO DISPLAY OLED ---
#define LARGURA_TELA 128
#define ALTURA_TELA 32
#define PINO_RESET -1

// Instancia o display apontando para o barramento '&Wire'
Adafruit_SSD1306 display(LARGURA_TELA, ALTURA_TELA, &Wire, PINO_RESET);

// --- CONFIGURAÇÕES DO SENSOR E ADC ---
Adafruit_ADS1115 ads;
const float RESISTOR_BURDEN = 22.0; 
const float RELACAO_SENSOR = 2000.0;
const float FATOR_TENSAO_ADS = 0.125; // mV por bit no GAIN_ONE
const float ICAL = 105.0;

void setup() {
  Serial.begin(115200);
  
  // 1. Inicializa os dois barramentos físicos com seus respectivos pinos
  Wire.begin(26, 25);     // I2C0: SDA = 26, SCL = 25 (Para o Display)
  I2C_ADS.begin(33, 32);  // I2C1: SDA = 33, SCL = 32 (Para o ADS1115)
  
  // 2. Inicializa o Display OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Falha ao inicializar o SSD1306!");
    while (1);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 10);
  display.println("Iniciando...");
  display.display();
  delay(1000);

  if (!ads.begin(0x48, &I2C_ADS)) {
    Serial.println("Falha ao inicializar o ADS1115!");
    display.clearDisplay();
    display.setCursor(10, 10);
    display.println("Erro no ADS1115!");
    display.display();
    while (1); 
  }
  
  ads.setGain(GAIN_ONE);
  ads.setDataRate(RATE_ADS1115_860SPS);
}

void loop() {
  // Faz a leitura da corrente durante 200 milissegundos
  float corrente = calcularCorrenteRMS(1000);

  // --- ATUALIZA O DISPLAY OLED ---
  display.clearDisplay(); 
  
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Corrente (SCT-013):");
  
  display.setTextSize(3); 
  display.setCursor(0, 10);
  display.print(corrente, 2); 
  
  display.setTextSize(2);
  display.print(" A");
  
  display.display(); 
  
  Serial.print("Corrente: ");
  Serial.print(corrente, 2);
  Serial.println(" A");
  
  delay(500);
}

// --- FUNÇÃO PARA CALCULAR A CORRENTE RMS ---
float calcularCorrenteRMS(uint32_t tempoAmostragem) {
  uint32_t tempoInicio = millis();
  
  double sumI = 0;
  int amostras = 0;
  
  static double offsetI = 13200.0; 

  while ((millis() - tempoInicio) < tempoAmostragem) {
    int16_t sampleI = ads.readADC_SingleEnded(0);

    offsetI = offsetI + ((sampleI - offsetI) / 1024.0);
    double filteredI = sampleI - offsetI;

    sumI += (filteredI * filteredI);
    amostras++;
  }

  double irms_adc = sqrt(sumI / amostras);

  float tensao_rms = (irms_adc * FATOR_TENSAO_ADS) / 1000.0;

  float correnteReal = tensao_rms * ICAL;

  if (correnteReal < 0.05) {
    correnteReal = 0.00;
  }

  return correnteReal;
}