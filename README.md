# ESP32-S3: Cliente HTTP con Open-Meteo y Comparativa Ambiental

Este proyecto implementa un **Cliente HTTP** en la placa **ESP32-S3** que consulta en tiempo real el clima exterior mediante la API REST pública de **Open-Meteo**, procesa la respuesta en formato JSON utilizando **ArduinoJson 7** con filtrado en memoria, y compara las mediciones externas (temperatura y humedad) con las lecturas de un sensor ambiental interno (Sensirion SHT3x) sobre el bus I2C.

---

## 🚀 Características principales
- **Cliente HTTP REST:** Realiza peticiones `GET` con `HTTPClient` configurado en `HTTP/1.0` para evitar problemas de fragmentación (*chunked transfer encoding*).
- **Procesamiento de JSON con Filtros:** Utiliza `ArduinoJson 7` con `DeserializationOption::Filter` para descartar los campos innecesarios del JSON en el flujo de entrada sin sobrecargar la memoria RAM de la placa.
- **Temporización no bloqueante:** Emplea `millis()` en lugar de `delay()` para consultar el servicio meteorológico cada 60 segundos sin detener la lectura del sensor ni la actualización de la pantalla.
- **Manejo de valores por defecto:** Usa el operador `| NAN` para distinguir la ausencia de datos o fallos de red sin inventar lecturas ficticias de 0.0 °C.
- **Salida dual:** Muestra los datos tanto en pantalla OLED SSD1306 como en el Monitor Serie a 115200 baudios.

---

## 🛠️ Requisitos
- Placa de desarrollo **ESP32-S3**.
- [Visual Studio Code](https://code.visualstudio.com/) con **PlatformIO IDE**.
- Red WiFi local con acceso a internet (2.4 GHz).
- *(Opcional)* Sensor Sensirion SHT3x y pantalla OLED SSD1306 (`SDA: GPIO 8`, `SCL: GPIO 9`). Si no están conectados, el firmware sigue operando y muestra los datos del clima exterior en la consola serie.

---

## 🚀 Cómo ejecutar el proyecto

### 1. Configurar credenciales WiFi
1. Copia el archivo `include/secretos.h.ejemplo` como `include/secretos.h`:
   ```c
   #pragma once

   #define WIFI_SSID   "TuNombreDeRedWiFi"
   #define WIFI_CLAVE  "TuContraseñaDeWiFi"
   ```

### 2. (Opcional) Ajustar coordenadas geográficas
En `src/main.cpp`, puedes modificar las coordenadas de latitud y longitud según tu ubicación (por defecto configuradas para Tuluá, Colombia):
```cpp
#define LAT    4.0847
#define LON  -76.1954
```

### 3. Compilar y subir
- Conecta la placa por USB.
- Haz clic en la flecha **`→`** (*Upload*) en la barra inferior de PlatformIO.
- Haz clic en el enchufe **`🔌`** (*Serial Monitor*) a **115200 baudios**.

### 4. Salida en el Monitor Serie
Al arrancar, el ESP32 se conectará a la red WiFi y consultará la API pública de Open-Meteo:
```text
WiFi conectado. IP local: 192.168.1.XX

Consultando servicio Open-Meteo...
========================================
 Clima afuera (Open-Meteo):
 -> Temperatura: 23.4 °C
 -> Humedad:     82 %
========================================
```
Cada 60 segundos el firmware actualizará los datos automáticamente.
