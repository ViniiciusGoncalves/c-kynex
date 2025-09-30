#include <WiFi.h>
#include <HTTPClient.h>
#include <HX711.h>
#include <DHT.h> // <-- BIBLIOTECA ADICIONADA PARA O DHT22

// --- Configurações dos Pinos ---

//--- Sensor de Temperatura e Umidade (DHT22) ---
#define DHT_PIN 2  // <-- NOVO PINO PARA O SENSOR DHT22. ESCOLHA UMA GPIO VÁLIDA!
#define DHT_TYPE DHT22

//--- Sensor de Peso (HX711) ---
#define DT_PIN 5
#define SCK_PIN 18

//--- Sensor de Vibração (SW-420) ---
#define VIBR_PIN 27

//--- Sensor de Corrente (ACS712) ---
#define CORRENTE_PIN 25

// --- Constantes e Variáveis Globais ---

// Constante para calibração da balança
const float FATOR_CALIBRACAO = -4652.0;

// Constantes para o sensor de corrente ACS712
const float sensibilidade = 0.100;      // Sensibilidade para o modelo de 20A (100mV/A)
const float tensaoReferenciaADC = 3.3;  // Tensão de operação do ESP32
const int resolucaoADC = 4095;          // Resolução do ADC do ESP32 (12-bit)
const float vccSensor = 3.3;            // Tensão de alimentação do MÓDULO ACS712
const float tensaoOffset = vccSensor / 2.0;

// --- Configurações de Rede ---
//const char* ssid = "Jvc";
//const char* password = "Jjvictor2302#7394";
const char* ssid = "CN";
const char* password = "Csantos33";
String serverName = "http://192.168.15.33:5001";

// --- Instâncias dos Sensores ---
DHT dht(DHT_PIN, DHT_TYPE);
HX711 balanca;

void setup() {
  Serial.begin(9600);

  // --- Inicialização do Sensor de Peso ---
  balanca.begin(DT_PIN, SCK_PIN);
  balanca.set_scale(FATOR_CALIBRACAO);
  balanca.tare();
  Serial.println("Balança pronta para uso.");

  // --- Inicialização do Sensor de Temperatura e Umidade ---
  dht.begin();
  Serial.println("Sensor DHT22 inicializado.");

  // --- Inicialização do pino do Sensor de Vibração ---
  pinMode(VIBR_PIN, INPUT);
  Serial.println("Sensor de Vibração inicializado.");

  // --- Conexão Wi-Fi ---
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }
  Serial.println("WiFi conectado!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Verifica se o WiFi está conectado antes de tentar enviar os dados
  if (WiFi.status() == WL_CONNECTED) {
    // Chama as funções para ler e enviar os dados de cada sensor
    enviarDadosPeso();
    delay(500); // Pequeno delay para não sobrecarregar o servidor com requisições simultâneas
    enviarDadosTemperaturaUmidade();
    delay(500);
    enviarDadosVibracao();
    delay(500);
    enviarDadosCorrente();
  } else {
    Serial.println("WiFi desconectado. Tentando reconectar...");
    WiFi.begin(ssid, password);
  }

  // Intervalo principal entre os ciclos de leitura e envio de TODOS os sensores
  Serial.println("\n--- Ciclo completo. Aguardando 10 segundos... ---\n");
  delay(10000);
}

/**
 * @brief Lê o sensor de peso (HX711) e envia os dados para o endpoint /peso.
 */
void enviarDadosPeso() {
  float peso = balanca.get_units(10);
  Serial.print("Peso: ");
  Serial.print(peso, 2);
  Serial.println(" kg");

  HTTPClient http;
  String urlCompleta = serverName + "/peso";
  http.begin(urlCompleta);
  http.addHeader("Content-Type", "application/json");

  String jsonData = "{\"peso\": " + String(peso, 2) + "}";

  int httpResponseCode = http.POST(jsonData);

  if (httpResponseCode > 0) {
    Serial.print("Peso - Código de resposta HTTP: ");
    Serial.println(httpResponseCode);
  } else {
    Serial.print("Peso - Erro na requisição: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

/**
 * @brief Lê o sensor de temperatura e umidade (DHT22) e envia os dados para o endpoint /temperatura_umidade.
 */
void enviarDadosTemperaturaUmidade() {
  float umidade = dht.readHumidity();
  float temperatura = dht.readTemperature();

  // Verifica se a leitura falhou
  if (isnan(temperatura) || isnan(umidade)) {
    Serial.println("Falha na leitura do sensor DHT22!");
    return;
  }

  Serial.print("Temperatura: ");
  Serial.print(temperatura);
  Serial.print(" °C, Umidade: ");
  Serial.print(umidade);
  Serial.println(" %");

  HTTPClient http;
  // Endpoint para temperatura e umidade. Ajuste se necessário.
  String urlCompleta = serverName + "/temperatura_umidade"; 
  http.begin(urlCompleta);
  http.addHeader("Content-Type", "application/json");

  String jsonData = "{\"temperatura\": " + String(temperatura) + ", \"umidade\": " + String(umidade) + "}";

  int httpResponseCode = http.POST(jsonData);

  if (httpResponseCode > 0) {
    Serial.print("Temp/Umidade - Código de resposta HTTP: ");
    Serial.println(httpResponseCode);
  } else {
    Serial.print("Temp/Umidade - Erro na requisição: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

/**
 * @brief Lê o sensor de vibração (SW-420) e envia os dados para o endpoint /vibracao.
 */
void enviarDadosVibracao() {
  // A leitura digital retorna LOW (0) quando há vibração e HIGH (1) quando não há.
  int estadoDigital = digitalRead(VIBR_PIN); 
  // A leitura analógica dá uma ideia da intensidade (valor menor = mais vibração)
  int valorAnalogico = analogRead(VIBR_PIN); 

  String estadoStr = (estadoDigital == LOW) ? "Vibracao Detectada" : "Normal";

  Serial.print("Vibração - Estado: ");
  Serial.print(estadoStr);
  Serial.print(", Valor Analógico: ");
  Serial.println(valorAnalogico);
  
  HTTPClient http;
  String urlCompleta = serverName + "/vibracao";
  http.begin(urlCompleta);
  http.addHeader("Content-Type", "application/json");

  String jsonData = "{\"estado_vibracao\": \"" + estadoStr + "\", \"valor_analogico\": " + String(valorAnalogico) + "}";

  int httpResponseCode = http.POST(jsonData);

  if (httpResponseCode > 0) {
    Serial.print("Vibração - Código de resposta HTTP: ");
    Serial.println(httpResponseCode);
  } else {
    Serial.print("Vibração - Erro na requisição: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

/**
 * @brief Lê o sensor de corrente (ACS712) e envia os dados para o endpoint /corrente.
 */
void enviarDadosCorrente() {
  int valorADC = analogRead(CORRENTE_PIN);
  float tensaoNoPino = (valorADC / (float)resolucaoADC) * tensaoReferenciaADC;
  float corrente = (tensaoNoPino - tensaoOffset) / sensibilidade;

  // Corrige pequenas flutuações em torno de zero
  if (abs(corrente) < 0.1) {
    corrente = 0.0;
  }

  Serial.print("Corrente: ");
  Serial.print(corrente, 2);
  Serial.println(" A");

  HTTPClient http;
  String urlCompleta = serverName + "/corrente";
  http.begin(urlCompleta);
  http.addHeader("Content-Type", "application/json");

  String jsonData = "{\"corrente\": " + String(corrente, 2) + "}";

  int httpResponseCode = http.POST(jsonData);

  if (httpResponseCode > 0) {
    Serial.print("Corrente - Código de resposta HTTP: ");
    Serial.println(httpResponseCode);
  } else {
    Serial.print("Corrente - Erro na requisição: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}