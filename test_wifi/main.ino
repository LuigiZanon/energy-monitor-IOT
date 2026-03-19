#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define LARGURA_TELA 128
#define ALTURA_TELA 64
#define PINO_RESET -1

WebServer server(80);
Adafruit_SSD1306 display(LARGURA_TELA, ALTURA_TELA, &Wire, PINO_RESET);

void setup_esp();
void connectWifi();
void run();
void verifyDevices();
void updateDisplayConfig();
void resetConfigMode();

enum State {
  CONFIG,       // abre Wi-Fi + servidor
  CONNECTING_WIFI,   // tenta conectar
  RUNNING        // roda aplicação
};

State state = CONFIG;

const char* espSSID = "ESP_Config";
const char* espPassword = "";

// ===== CONFIG DO ACCESS POINT =====
String homeSSID = "";
String homePassword = "";

int connectedDevices = -1;

// ===== SETUP =====
void setup() {
  Serial.begin(115200);

  Wire.begin(26, 25);    // I2C0: SDA = 25, SCL = 26 (Para o Display)
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Falha ao inicializar o SSD1306!");
    while (1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 20);
  display.println("Iniciando...");
  display.display();
  delay(1000);

  setup_esp();
}

// ===== LOOP =====
void loop() {
  switch(state) {
    case CONFIG:
      server.handleClient();
      verifyDevices();
      break;

    case CONNECTING_WIFI:
      connectWifi();
      break;

    case RUNNING:
      run();
      break;
  }
  
}

// ========= MODO DE CONFIGURAÇÃO
String htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP Setup</title>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; text-align: center; }
    input { padding: 10px; margin: 5px; width: 80%; }
    button { padding: 10px 20px; }
  </style>
</head>
<body>
  <h2>Configurar WiFi</h2>
  <form action="/save">
    <input type="text" name="ssid" placeholder="SSID"><br>
    <input type="password" name="pass" placeholder="Senha"><br>
    <button type="submit">Salvar</button>
  </form>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleSave() {
  homeSSID = server.arg("ssid");
  homePassword = server.arg("pass");

  Serial.println("Novo SSID: " + homeSSID);
  Serial.println("Nova Senha: " + homePassword);

  state = CONNECTING_WIFI;

  server.send(200, "text/html", "<h1>Salvo! Reinicie o dispositivo.</h1>");
}

void setup_esp() {
  // Verificar se existe salvo no SD

  // Cria o Access Point
  WiFi.softAP(espSSID, espPassword);

  Serial.println("AP criado!");
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  // Rotas
  server.on("/", handleRoot);
  server.on("/save", handleSave);

  server.begin();
  Serial.println("Servidor HTTP iniciado");
  
  connectedDevices = WiFi.softAPgetStationNum();
  updateDisplayConfig();

}

void updateDisplayConfig() {
  display.clearDisplay();

  if (connectedDevices == 0) {

    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Conecte-se ao wifi");

    display.setTextSize(2);
    display.setCursor(0, 25);
    display.print(espSSID);

  } else {

    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Acesse o IP:");

    display.setTextSize(2);
    display.setCursor(0, 25);
    display.print(WiFi.softAPIP());

  }

  display.display();
}

void verifyDevices() {
  int count = WiFi.softAPgetStationNum();

  if (count != connectedDevices) {
    connectedDevices = count;
    updateDisplayConfig();
  }
}

unsigned long lastAttempt = 0;
int retries = 0;

void resetConfigMode() {
  Serial.println("Voltando para modo CONFIG");

  // Para Wi-Fi atual (station)
  WiFi.disconnect(true);

  // Desliga AP antigo (se existir)
  WiFi.softAPdisconnect(true);

  delay(500);

  // Reseta variáveis
  connectedDevices = -1;

  // Sobe AP novamente
  WiFi.softAP(espSSID, espPassword);

  Serial.println("AP recriado!");
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  // Atualiza estado
  state = CONFIG;

  // Atualiza display
  updateDisplayConfig();
}

void connectWifi() {
  if (retries == 0) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(homeSSID.c_str(), homePassword.c_str());

    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 25);
    display.print("Conectando...");
    display.display();
  }

  if (millis() - lastAttempt > 500) {
    lastAttempt = millis();
    retries++;
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Conectado!");
    state = RUNNING;
    retries = 0;
  }

  if (retries >= 20) {
    Serial.println("Falhou!");

    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Rede incorreta");

    display.setCursor(0, 20);
    display.print("Tente novamente");

    display.display();

    delay(1500);

    retries = 0;
    resetConfigMode();
  }
}

// == MODO DE EXECUÇÃO
void run(){
  display.clearDisplay();

  display.setCursor(0, 20);
  display.print("Executando");

  display.display();
}