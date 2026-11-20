1. Introducción
Nivel: 🟢 Principiante · Duración estimada: 65 minutos

En el codelab anterior la placa esperaba a que alguien la llamara. Ahora hace lo contrario: ella llama. Consulta un servicio de clima en internet, interpreta la respuesta y la pone en la pantalla junto a su propia medición.

Lo que se construye
Una pantalla con tres líneas: lo que mide el sensor dentro del aula, lo que informa el servicio para la ciudad, y la diferencia entre las dos. Es la primera vez en la sesión que el dispositivo combina un dato propio con un dato ajeno, que es de lo que trata la integración.

Por qué este servicio y no otro
Se usa Open-Meteo. Tres razones, y las tres importan en un taller:

No pide clave. No hay que registrarse ni repartir un token entre veinte estudiantes.
Responde por HTTP simple, sin obligar a cifrado, que en un microcontrolador cuesta memoria.
La respuesta es pequeña y estable: unos cuatrocientos bytes con los campos justos.
Objetivos
Al terminar este codelab se sabe:

Probar una llamada a un servicio desde la terminal antes de programarla.
Pedir datos a un servicio desde la placa y comprobar que la respuesta sirve.
Interpretar un JSON con ArduinoJson quedándose solo con los campos que interesan.
Repetir una tarea cada cierto tiempo sin detener el programa.
Decir qué protege y qué no protege el cifrado en un dispositivo de este tamaño.
Antes de empezar
El codelab de WiFi funciona en modo estación con salida a internet
El sensor y la pantalla responden}

2. La petición, antes de programarla
Toda llamada a un servicio se prueba primero desde el computador. Si no funciona ahí, no vale la pena buscar el error en el microcontrolador.

curl "http://api.open-meteo.com/v1/forecast?latitude=4.0847&longitude=-76.1954&current=temperature_2m,relative_humidity_2m&timezone=auto"
La respuesta, con las coordenadas de Tuluá:

{"latitude":4.0421796,"longitude":-76.210266,"generationtime_ms":0.0344,
 "utc_offset_seconds":-18000,"timezone":"America/Bogota","timezone_abbreviation":"GMT-5",
 "elevation":979.0,
 "current_units":{"time":"iso8601","interval":"seconds",
                  "temperature_2m":"°C","relative_humidity_2m":"%"},
 "current":{"time":"2026-11-12T21:45","interval":900,
            "temperature_2m":22.9,"relative_humidity_2m":87}}
La llamada al servicio desde la terminal de VS Code

Las partes de la dirección
Parte

Qué significa

api.open-meteo.com

el servidor al que se llama

/v1/forecast

el recurso que se pide

latitude y longitude

el punto del planeta, en grados decimales

current=...

qué variables se quieren, separadas por comas

timezone=auto

que las horas vengan en la zona horaria del punto pedido

Lo que va después del signo de interrogación son los parámetros de consulta: la forma estándar de afinar una petición GET. Es exactamente lo mismo que hace un buscador cuando se escribe algo en su caja.

Las coordenadas del aula se sacan de cualquier mapa: clic derecho sobre el punto y aparecen los dos números. Tuluá queda alrededor de 4,0847 de latitud y −76,1954 de longitud. El servicio responde con las coordenadas del punto de su malla más cercano, que es por lo que no coinciden exactamente con las pedidas.

Checkpoint
El curl responde con un bloque current que trae temperatura y humedad

3. Pedirlo desde la placa
#include <HTTPClient.h>

bool consultarClima() {
  if (WiFi.status() != WL_CONNECTED) return false;

  String url = String("http://api.open-meteo.com/v1/forecast?latitude=") + String(LAT, 4) +
               "&longitude=" + String(LON, 4) +
               "&current=temperature_2m,relative_humidity_2m&timezone=auto";

  WiFiClient red;
  HTTPClient http;
  http.setTimeout(8000);
  http.useHTTP10(true);          // el cuerpo de una pieza, sin trocear
  if (!http.begin(red, url)) { Serial.println("URL mal formada"); return false; }

  int codigo = http.GET();
  if (codigo != HTTP_CODE_OK) {
    Serial.printf("El servicio respondió %d\n", codigo);
    http.end();
    return false;
  }
  // ... aquí se interpreta la respuesta ...
  http.end();
  return true;
}
Cómo se arma la dirección
String url = String("http://api.open-meteo.com/v1/forecast?latitude=") + String(LAT, 4) + ...
La dirección no es fija: lleva dentro las coordenadas, que son números. String(LAT, 4) convierte el número en texto con cuatro decimales, y el + va pegando los trozos uno detrás de otro hasta formar la dirección completa.

El primer String(...) del todo no es decorativo: obliga a que la suma se haga entre textos. Sin él, el + intentaría sumar punteros y el resultado sería basura.

Los números de las coordenadas se escriben siempre con punto decimal, nunca con coma: 4.0847. Con coma, el servicio responde 400 y la causa no es evidente mirando el código.

Por qué se pide HTTP/1.0
http.useHTTP10(true) es una línea corta que evita un fallo silencioso. En HTTP/1.1 el servidor puede mandar el cuerpo troceado —la cabecera Transfer-Encoding: chunked—: antes de cada trozo escribe su tamaño en hexadecimal y cierra con un cero. http.getString() deshace ese envoltorio; http.getStream(), que es lo que se le pasa a ArduinoJson, no lo deshace.

El resultado es el peor que puede darse: el intérprete lee el tamaño del primer trozo como si fuera el documento entero, se da por satisfecho, no devuelve ningún error, y las dos medidas salen nan. Pedir HTTP/1.0 lo evita de raíz, porque esa versión del protocolo no tiene troceado.

Open-Meteo responde troceado: se comprueba con curl -i y ahí está la cabecera Transfer-Encoding: chunked. Sin useHTTP10(true) el programa compila, se conecta, recibe un 200 correcto y escribe nan en la pantalla sin quejarse de nada.

Las cuatro comprobaciones
Cada una evita un fallo distinto, y las cuatro son el motivo por el que esta función tiene el doble de líneas que la versión de tutorial:

¿Hay WiFi? Llamar sin red devuelve un error raro y tarda. Se pregunta antes.
¿La URL es válida? begin() devuelve false si está mal escrita.
¿El servidor respondió 200? Un 404 o un 500 también traen cuerpo; interpretarlo como si fueran datos buenos es la forma más silenciosa de mostrar basura en pantalla.
Llamar a http.end() siempre. Libera el socket. Sin eso, a las pocas decenas de llamadas el dispositivo se queda sin conexiones y deja de funcionar sin decir por qué.
Los códigos que devuelve GET
Los positivos son los códigos HTTP de siempre: 200 correcto, 404 no existe, 500 error del servidor. Los negativos son errores de la propia librería, antes de que hubiera respuesta:

Código

Qué pasó

−1

no se pudo abrir la conexión: DNS, red caída o servidor inalcanzable

−11

se agotó el tiempo de espera

−5

la conexión se perdió a mitad de la respuesta

En la red del campus suele haber un proxy o un filtro que corta el tráfico de los equipos que no son del dominio. Si el curl funciona en el portátil y la placa recibe −1, antes de revisar el código conviene probar con el punto de acceso de un celular.


4. Interpretar el JSON
La respuesta llega como texto. Convertirla en números es trabajo de ArduinoJson, y esa librería todavía no está en el proyecto: hay que añadirla.

Se agrega una línea más a lib_deps, en platformio.ini:

lib_deps =
	sensirion/arduino-sht@^2.0.0
	adafruit/Adafruit SSD1306@^2.5.13
	adafruit/Adafruit GFX Library@^1.11.11
	bblanchon/ArduinoJson@^7.4.3
Al guardar y volver a compilar, PlatformIO la descarga sola. Si falta esa línea, el compilador se detiene con fatal error: ArduinoJson.h: No such file or directory.

La versión importa. Este codelab usa la forma de la versión 7, en la que el documento se declara JsonDocument doc;, sin tamaño. En la versión 6 había que escribir StaticJsonDocument<512> doc; y reservar el espacio a mano, así que el mismo código no compila. Por eso la línea termina en @^7.4.3.

#include <ArduinoJson.h>

JsonDocument doc;
JsonDocument filtro;
filtro["current"]["temperature_2m"] = true;
filtro["current"]["relative_humidity_2m"] = true;

DeserializationError err = deserializeJson(doc, http.getStream(),
                                           DeserializationOption::Filter(filtro));
if (err) { Serial.printf("JSON ilegible: %s\n", err.c_str()); return false; }

float tempFuera = doc["current"]["temperature_2m"] | NAN;
float humFuera  = doc["current"]["relative_humidity_2m"] | NAN;
Qué hace el filtro
Le dice a la librería qué campos conservar; todo lo demás se descarta mientras se lee, sin llegar a ocupar memoria.

Con esta respuesta de cuatrocientos bytes no haría falta. Hace falta en cuanto se pide algo más: añadir &hourly=temperature_2m al mismo servicio devuelve un pronóstico de días completos, decenas de kilobytes, y sin filtro el documento no cabe. Conviene adoptarlo desde el principio, porque el día que la respuesta crezca el programa ya estará preparado.

Qué hace la barra vertical
float t = doc["current"]["temperature_2m"] | NAN;
Es el valor por defecto de ArduinoJson: si el campo no existe o no es un número, en lugar de devolver cero devuelve lo que está a la derecha. Un cero inventado es indistinguible de una temperatura de cero grados; NAN se puede detectar con isnan() y mostrar como «sin datos».

Leer del flujo y no de una cadena
deserializeJson(doc, http.getStream(), ...) interpreta la respuesta a medida que llega. La alternativa, http.getString(), primero guarda toda la respuesta en una cadena y después la interpreta: durante un instante, el mismo contenido está dos veces en memoria. Con respuestas grandes, eso basta para reiniciar la placa.

Checkpoint
La consola imprime la temperatura y la humedad de afuera
Al poner mal el nombre de un campo, el valor sale como nan y no como 0.00

5. El programa completo
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

#define LAT    4.0847      // coordenadas del aula, en grados decimales
#define LON  -76.1954

Adafruit_SSD1306 pantalla(128, 32, &Wire, -1);
SHTSensor        sensor;

float tempFuera = NAN, humFuera = NAN;
unsigned long ultimaConsulta = 0;
const unsigned long CADA = 60000;   // una consulta por minuto

bool consultarClima() {
  if (WiFi.status() != WL_CONNECTED) return false;

  String url = String("http://api.open-meteo.com/v1/forecast?latitude=") + String(LAT, 4) +
               "&longitude=" + String(LON, 4) +
               "&current=temperature_2m,relative_humidity_2m&timezone=auto";

  WiFiClient red;
  HTTPClient http;
  http.setTimeout(8000);
  http.useHTTP10(true);          // el cuerpo de una pieza, sin trocear
  if (!http.begin(red, url)) { Serial.println("URL mal formada"); return false; }

  int codigo = http.GET();
  if (codigo != HTTP_CODE_OK) {
    Serial.printf("El servicio respondió %d\n", codigo);
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
  if (err) { Serial.printf("JSON ilegible: %s\n", err.c_str()); return false; }

  tempFuera = doc["current"]["temperature_2m"] | NAN;
  humFuera  = doc["current"]["relative_humidity_2m"] | NAN;
  Serial.printf("Fuera: %.1f C  %.0f %%\n", tempFuera, humFuera);
  return true;
}

void pintar(float tDentro, float hDentro) {
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
  if (!sensor.init()) Serial.println("Sensor no encontrado: no habrá medida de adentro.");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_CLAVE);
  Serial.print("Conectando");
  unsigned long t0 = millis();                    // el tiempo límite de siempre
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("IP: "); Serial.println(WiFi.localIP());
  } else {
    Serial.println("Sin WiFi: se mostrará solo la medida de adentro.");
  }

  consultarClima();
  ultimaConsulta = millis();
}

void loop() {
  if (millis() - ultimaConsulta >= CADA) {
    ultimaConsulta = millis();
    consultarClima();
  }
  if (sensor.readSample()) pintar(sensor.getTemperature(), sensor.getHumidity());
  else                     Serial.println("El sensor de adentro no contestó");
  delay(2000);
}
Salida típica en la consola, con una consulta nueva cada minuto:

Conectando..
IP: 192.168.1.28
Fuera: 22.9 C  87 %
Fuera: 22.9 C  87 %
La consola con una consulta nueva cada minuto

Y en la pantalla, las tres líneas:

dentro 28.8C 53%
fuera  22.9C 87%
dif    +5.9C
Las coordenadas van arriba del programa, junto a los pines, y no en secretos.h: ese archivo existe para lo que no puede salir del computador, y unas coordenadas no son un secreto. Metidas ahí, además, quien clone el repositorio se queda sin ellas, porque secretos.h está en el .gitignore.

Por qué millis y no delay
Lo cómodo sería delay(60000) al final del loop(). Y entonces, durante un minuto entero, el programa no haría nada más: ni actualizar la pantalla, ni leer el sensor, ni atender a nadie.

Con millis() el programa pregunta «¿ya pasó un minuto desde la última consulta?» y, si no, sigue con lo suyo. Es el patrón básico de cualquier firmware que tenga que hacer dos cosas a ritmos distintos: el sensor cada dos segundos, el servicio cada minuto.

La resta millis() - ultimaConsulta se escribe siempre así, nunca como millis() > ultimaConsulta + CADA. El contador de milisegundos se desborda a los 49 días y vuelve a cero; con la resta entre enteros sin signo la cuenta sigue saliendo bien, con la suma no.

Checkpoint
La pantalla muestra las tres líneas: dentro, fuera y diferencia
Al soplar sobre el sensor cambia la primera línea y la diferencia, no la segunda
La consola imprime una consulta nueva cada minuto
El ejercicio
Cambiar las coordenadas por las de otra ciudad y comprobar que la segunda línea cambia.
Escribir mal el nombre de un campo —temperature_2x— y ver que la pantalla dice «sin datos» en lugar de inventar un cero.
Apagar el WiFi del router un minuto y observar que el programa sigue mostrando la medida de adentro.
Añadir &current=wind_speed_10m a la dirección y mostrar también la velocidad del viento.
6. Cada cuánto se puede preguntar
Un minuto no es un número caprichoso.

El servicio actualiza sus datos cada quince minutos: preguntar cada segundo devolvería exactamente lo mismo mil veces. Además, los servicios abiertos limitan las peticiones por dirección IP y, si se pasan, cortan: primero responden 429 y después dejan de responder del todo.

Para un dato que cambia despacio —el clima, el precio de una divisa, el estado de un servidor— consultar cada minuto ya es generoso. La regla práctica: preguntar al ritmo al que cambia el dato, no al ritmo al que corre el bucle.

7. HTTP y HTTPS en un microcontrolador
Esta práctica va por HTTP, sin cifrar, y conviene decir por qué y qué significa.

El dato es público y no viaja ninguna credencial: lo peor que puede hacer alguien que intercepte el tráfico es enterarse de la temperatura de Tuluá. Pero cualquiera en el camino podría alterar la respuesta, y el dispositivo la creería.

Hacerlo por HTTPS exige tres cosas que no son gratis en una placa:

Guardar el certificado raíz de la autoridad que firma el del servidor, y actualizarlo cuando caduque.
Memoria libre para el apretón de manos TLS: unos 40 KB en el peor momento.
Tener el reloj en hora, porque validar un certificado es comprobar fechas. Sin sincronizar la hora por NTP, la validación falla siempre.
En los ejemplos de internet abunda client.setInsecure(), que acepta cualquier certificado sin comprobarlo. Eso cifra el tráfico pero no verifica con quién se está hablando: deja pasar exactamente el ataque del que protege el HTTPS. Sirve para una prueba de dos minutos; en un dispositivo que se deja instalado, no.

El codelab de MQTT con TLS de esta serie hace el camino completo: certificado, hora por NTP y verificación real.

8. Errores frecuentes
Síntoma

Causa habitual

GET devuelve −1

sin salida a internet, DNS bloqueado, o filtro de la red del campus

GET devuelve −11

el servicio tardó más que el tiempo límite; subir a 10 s o reintentar

GET devuelve 400

falta un parámetro en la URL o las coordenadas van con coma decimal

Las dos medidas salen nan y el servicio respondió 200

falta http.useHTTP10(true): la respuesta viene troceada

La temperatura sale 0.00

se leyó un campo que no existe y no se usó el valor por defecto

JSON ilegible: NoMemory

falta el filtro y la respuesta no cabe

La pantalla se congela

se usó delay(60000) en lugar de millis()

Funciona un rato y deja de responder

falta http.end() y se agotaron los sockets

Al cabo de horas se reinicia sola

fuga de memoria: cadenas que crecen dentro del loop()