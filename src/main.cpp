#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SHTSensor.h>
#include <Adafruit_SSD1306.h>
#include "secretos.h"

#define PIN_SDA 8
#define PIN_SCL 9

#define LAT    4.0847      // coordenadas de Tuluá (o tu ciudad), en grados decimales
#define LON  -76.1954

Adafruit_SSD1306 pantalla(128, 32, &Wire, -1);
SHTSensor        sensor;
bool             sensorPresente = false;

float tempFuera = NAN, humFuera = NAN;
unsigned long ultimaConsulta = 0;
const unsigned long CADA = 60000;   // una consulta por minuto

bool consultarClima() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Sin conexión WiFi: no se puede consultar Open-Meteo.");
    return false;
  }

  String url = String("http://api.open-meteo.com/v1/forecast?latitude=") + String(LAT, 4) +
               "&longitude=" + String(LON, 4) +
               "&current=temperature_2m,relative_humidity_2m&timezone=auto";

  Serial.println("\nConsultando servicio Open-Meteo...");
  WiFiClient red;
  HTTPClient http;
  http.setTimeout(8000);
  http.useHTTP10(true);          // el cuerpo de una pieza, sin trocear
  if (!http.begin(red, url)) { 
    Serial.println("URL mal formada"); 
    return false; 
  }

  int codigo = http.GET();
  if (codigo != HTTP_CODE_OK) {
    Serial.printf("El servicio respondió código de error HTTP: %d\n", codigo);
    http.end();
    return false;
  }

  JsonDocument doc;
  JsonDocument filtro;
  filtro["current"]["temperature_2m"] = true;
  filtro["current"]["relative_humidity_2m"] = true;
  DeserializationError err = deserializeJson(doc, http.getStream(),
                                             DeserializationOption::Filter(filtro));
  http.end();
  if (err) { 
    Serial.printf("JSON ilegible: %s\n", err.c_str()); 
    return false; 
  }

  tempFuera = doc["current"]["temperature_2m"] | NAN;
  humFuera  = doc["current"]["relative_humidity_2m"] | NAN;

  Serial.println("========================================");
  Serial.printf(" Clima afuera (Open-Meteo):\n");
  Serial.printf(" -> Temperatura: %.1f °C\n", tempFuera);
  Serial.printf(" -> Humedad:     %.0f %%\n", humFuera);
  Serial.println("========================================");
  return true;
}

void pintar(float tDentro, float hDentro) {
  // Simulación en monitor serie
  Serial.printf("[DENTRO] %.1f°C %.0f%% | [FUERA] %.1f°C %.0f%% | [DIF] %+.1f°C\n",
                tDentro, hDentro, tempFuera, humFuera, tDentro - tempFuera);

  pantalla.clearDisplay();
  pantalla.setTextColor(SSD1306_WHITE);
  pantalla.setTextSize(1);
  pantalla.setCursor(0, 0);
  pantalla.printf("dentro %.1fC %.0f%%", tDentro, hDentro);
  pantalla.setCursor(0, 11);
  if (isnan(tempFuera)) pantalla.print("fuera  sin datos");
  else                  pantalla.printf("fuera  %.1fC %.0f%%", tempFuera, humFuera);
  pantalla.setCursor(0, 23);
  if (!isnan(tempFuera)) pantalla.printf("dif    %+.1fC", tDentro - tempFuera);
  pantalla.display();
}

void setup() {
  Serial.begin(115200);
  delay(400);
  Wire.begin(PIN_SDA, PIN_SCL);
  pantalla.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  if (sensor.init()) {
    sensorPresente = true;
    Serial.println("Sensor SHT detectado correctamente.");
  } else {
    sensorPresente = false;
    Serial.println("Sensor SHT no encontrado: se mostrará la información del clima exterior.");
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_CLAVE);
  Serial.print("Conectando a ");
  Serial.print(WIFI_SSID);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi conectado. IP local: "); 
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Sin WiFi: no se podrá consultar el clima exterior.");
  }

  // Primera consulta inmediata
  consultarClima();
  ultimaConsulta = millis();
}

void loop() {
  // Consulta periódica cada 60 segundos sin bloquear el loop
  if (millis() - ultimaConsulta >= CADA) {
    ultimaConsulta = millis();
    consultarClima();
  }

  if (sensorPresente && sensor.readSample()) {
    pintar(sensor.getTemperature(), sensor.getHumidity());
    delay(2000);
  } else {
    delay(1000);
  }
}
