#include <WiFi.h>
#include <WiFiManager.h>         
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <PubSubClient.h>
#include <Adafruit_ADS1X15.h>

#define LARGURA_TELA 128
#define ALTURA_TELA 32
#define PINO_RESET -1
#define MAX_BATCH 10
#define BTN_RESET 5 // Valor aleatório enquanto nao coloca

TwoWire I2C_ADS = TwoWire(1);
Adafruit_SSD1306 display(LARGURA_TELA, ALTURA_TELA, &Wire, PINO_RESET);
WiFiManager wm;
WiFiClient espClient;
PubSubClient client(espClient);

const char* SSID = "EnergyMonitor"; 
const char* password = "";
const char* mqtt_server = "192.168.68.115";
const char* mqtt_user = "";
const char* mqtt_password = "";
const char* mqtt_topic = "teste/esp";
const int mqtt_port = 1883;

Adafruit_ADS1115 ads;
const float RESISTOR_BURDEN = 22.0; 
const float RELACAO_SENSOR = 2000.0; 
const float FATOR_TENSAO_ADS = 0.125; 

struct __attribute__((packed)) Sample {
  float current;
};

Sample buffer[MAX_BATCH];
int count = 0;
void addData(float current) {
    buffer[count].current = current;
    count++;

    if (count == MAX_BATCH) {
        // client.publish(mqtt_topic, (uint8_t*)buffer, sizeof(buffer));
        client.publish(mqtt_topic, (uint8_t*)buffer, count * sizeof(Sample));
        count = 0;
    }
}

// ===== CALLBACK MQTT =====
void callback(char* topic, byte* payload, unsigned int length) {
  // Serial.print("Mensagem recebida [");
  // Serial.print(topic);
  // Serial.print("]: ");

  // for (int i = 0; i < length; i++) {
  //   Serial.print((char)payload[i]);
  // }
  // Serial.println();
}

// ===== RECONEXÃO MQTT =====
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT...");

    String clientId = "ESPClient-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("conectado!");

      // Subscribe em um tópico
      client.subscribe(mqtt_topic);

    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}


void setup() {
  Serial.begin(115200);

  // pinMode(BTN_RESET, INPUT);

  // Configuração dos I2Cs
  I2C_ADS.begin(33, 32); // I2C1: SDA = 32, SCL = 33 (Para o ADS1115)
  Wire.begin(26, 25);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Falha ao inicializar o SSD1306!");
    while(1);
  }

  if (!ads.begin(0x48, &I2C_ADS)) {
    Serial.println("Falha ao inicializar o ADS1115!");
    display.clearDisplay();
    display.setCursor(10, 20);
    display.println("Erro no ADS1115!");
    display.display();
    while (1); 
  }

  ads.setGain(GAIN_ONE);
  ads.setDataRate(RATE_ADS1115_860SPS); 

  // Configuração do Wifi

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Iniciando...");
  display.display();
  delay(1000);

  // wm.resetSettings();

  wm.setAPCallback([](WiFiManager* wm){
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0,0);
    display.println("Modo Configuracao");
    display.print("WiFi: ");
    display.println(SSID);
    display.print("IP: ");
    display.println(WiFi.softAPIP()); // agora o IP estará correto
    display.display();
  });

  // Tenta conectar ao Wi-Fi salvo ou abre portal cativo
  if(!wm.autoConnect(SSID, password)) {
    Serial.println("Falhou conectar e timeout do portal cativo");
    display.clearDisplay();
    display.setCursor(0, 10);
    display.println("Falha WiFi");
    display.display();
    delay(3000);
    ESP.restart();
  }

  Serial.println("Wi-Fi conectado, iniciando MQTT...");
  client.setServer(mqtt_server, mqtt_port);

  client.setCallback(callback);

  Serial.println("Conectado ao WiFi!");
}

// --- FUNÇÃO PARA CALCULAR A CORRENTE RMS ---
float calcularCorrenteRMS(uint32_t tempoAmostragem) {
  uint32_t tempoInicio = millis();
  int32_t valorMaximo = 0;
  int32_t valorMinimo = 65535; 
  int16_t leituraADC = 0;

  while ((millis() - tempoInicio) < tempoAmostragem) {
    leituraADC = ads.readADC_SingleEnded(0); 
    if (leituraADC > valorMaximo) { valorMaximo = leituraADC; }
    if (leituraADC < valorMinimo) { valorMinimo = leituraADC; }
  }

  float tensaoPicoPico = ((valorMaximo - valorMinimo) * FATOR_TENSAO_ADS) / 1000.0;
  float tensaoRMS = (tensaoPicoPico / 2.0) * 0.707;
  float correnteSecundaria = tensaoRMS / RESISTOR_BURDEN;
  float correnteReal = correnteSecundaria * RELACAO_SENSOR;
  
  if (correnteReal < 0.15) { correnteReal = 0.0; }

  return correnteReal;
}

void loop() {
  // Código da sua aplicação
  if (!client.connected()) {
    reconnect();
  }

  client.loop();
  unsigned long now = millis();

  static unsigned long lastDisplay = 0;

  // Faz a leitura da corrente
  float corrente = calcularCorrenteRMS(200); 

  if (now - lastDisplay > 500) {
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

    addData(corrente);
  }

  // if(digitalRead(BTN_RESET) == LOW){
  //     WiFiManager wm;
  //     wm.resetSettings();
  //     ESP.restart();
  // }
  
  // // Publica mensagem a cada 5s
  // static unsigned long lastMsg = 0;

  // if (now - lastMsg > 5000) {
  //   lastMsg = now;

  //   String msg = "Olá do ESP!";
  //   Serial.print("Publicando: ");
  //   Serial.println(msg);

  //   client.publish(mqtt_topic, msg.c_str());
  // }
}
