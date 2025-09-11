#include <WiFi.h> 
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <DHT.h>

// 🧪 Sensores
#define DHTPIN 4            // Pino onde o DHT11 está conectado
#define DHTTYPE DHT11       // Tipo do sensor DHT
#define MQ135PIN 34         // Pino analógico do MQ135

DHT dht(DHTPIN, DHTTYPE);  // Inicializa o sensor DHT

// 🌐 WiFi (preencher com suas credenciais)
#define WIFI_SSID "SEU_WIFI"
#define WIFI_PASSWORD "SUA_SENHA"

// 📊 InfluxDB (ajuste conforme seu ambiente)
#define INFLUXDB_URL "http://SEU_SERVIDOR:8086"
#define INFLUXDB_TOKEN "SEU_TOKEN"
#define INFLUXDB_ORG "SUA_ORGANIZACAO"
#define INFLUXDB_BUCKET "sensores"

#define TZ_INFO "UTC-3" // Fuso horário para NTP

InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
Point sensorData("ambiente");

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 5000;  // Intervalo de envio: 5 segundos

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Conexão Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Conectando-se ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Conectado!");

  // Sincronizar horário
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  // Validar conexão com InfluxDB
  if (client.validateConnection()) {
    Serial.println("✅ Conectado ao InfluxDB!");
  } else {
    Serial.println("❌ Erro InfluxDB: " + client.getLastErrorMessage());
  }
}

void loop() {
  if (millis() - lastSendTime > sendInterval) {
    lastSendTime = millis();

    // Leitura dos sensores
    float temperatura = dht.readTemperature();
    float umidade = dht.readHumidity();
    int leituraMQ = analogRead(MQ135PIN);
    float fumaca = leituraMQ * (5.0 / 4095.0) * 1000.0;  // ppm aproximado

    // Log Serial
    Serial.printf("🌡️ Temp: %.2f°C | 💧 Umid: %.2f%% | 🔥 Fumaça: %.2f ppm\n", temperatura, umidade, fumaca);

    // Preencher ponto de dados
    sensorData.clearFields();
    sensorData.addField("temperatura", temperatura);
    sensorData.addField("umidade", umidade);
    sensorData.addField("fumaca", fumaca);

    // Enviar ao InfluxDB
    if (client.writePoint(sensorData)) {
      Serial.println("✅ Dados enviados ao InfluxDB!");
    } else {
      Serial.println("❌ Erro ao enviar dados: " + client.getLastErrorMessage());
    }
  }
}
