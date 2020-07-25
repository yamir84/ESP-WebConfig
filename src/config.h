#include <Arduino.h>
// Bibliotecas ------------------------------------------
#ifdef ESP8266
// Bibliotecas para ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266httpUpdate.h>
#include <FS.h>
#else
// Bibliotecas para ESP32
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPUpdate.h>
#include <SPIFFS.h>
#endif
#include <DNSServer.h>
#include <TimeLib.h>
#include <ArduinoJson.h>

/****************************************
 * Bibliotecas proyecto MQTT
 ****************************************/
#include <PubSubClient.h> //Libreria MQTT
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

/****************************************
 * Constantes & Variables
 ****************************************/
#define RELAY1 13                              //GPIO13 para salida de Relay
#define RELAY2 14                              //GPIO14 para salida de Relay
#define DHTPIN 4                               //GPIO4 para entrada sensor DHT
#define DHTTYPE DHT11                          //DHT 11

/****************************************
 * Constantes & Variables
 ****************************************/
const String device_id = "0000002"; // ID del Dispositivo
long dimmer;
long relay1;
char mqttBroker[] = "cubaelectronica.com"; // Direccíon del Brojer 192.168.0.15
char payload[150];                         // Tamaño del mensaje
char topico[150];                          // Tamaño del topico
char topico2[150];                         // Tamaño del topico2
long lastMsg = 0;
long lastReconnectAttempt = 0;             // Variable para no bloquear la reconexion del MQTT
/****************************************
 * Configuramos la IN1
 ****************************************/
struct Alarma
{
    const uint8_t PIN;
    bool active;
};
/****************************************/
Alarma alarma1 = {25, false};
int value1 = 0;
/****************************************
 * PWM
 ****************************************/
int freq = 5000;
int ledChannel = 0;
int resolution = 8;

/****************************************
 * Fin lubrerias y variables proyecto MQTT
 ****************************************/

// Constantes -------------------------------------------
// Porta Servidor Web
const byte WEBSERVER_PORT = 80;

// Headers do Servidor Web
const char *WEBSERVER_HEADER_KEYS[] = {"User-Agent"};

// Porta Servidor DNS
const byte DNSSERVER_PORT = 53;

// Pino do LED
#ifdef ESP32
// ESP32 não possui pino padrão de LED
const byte LED_PIN = 2;
#else
// ESP8266 possui pino padrão de LED
const byte LED_PIN = LED_BUILTIN;
#endif

// Estado do LED
#ifdef ESP8266
// ESP8266 utiliza o estado inverso
const byte LED_ON = LOW;
const byte LED_OFF = HIGH;
#else
// ESP32 utilizam estado normal
const byte LED_ON = HIGH;
const byte LED_OFF = LOW;
#endif

// Tamaño del Objeto JSON
const size_t JSON_SIZE = JSON_OBJECT_SIZE(8) + 130;

// Instancias -------------------------------------------
// Web Server
#ifdef ESP8266
// Classe WebServer para ESP8266
ESP8266WebServer server(WEBSERVER_PORT);
#else
// Classe WebServer para ESP32
WebServer server(WEBSERVER_PORT);
#endif

// DNS Server
DNSServer dnsServer;

// Variables Globales ------------------------------------
char id[30];    // Identificação do dispositivo
boolean ledOn;  // Estado do LED
word bootCount; // Número de inicializações
char ssid[30];  // Rede WiFi
char pw[30];    // Senha da Rede WiFi
char mqttuser[30];    // Usuario de MQTT Broker
char mqttpass[30];    // Password del Usuario MQTT Broker
char buffer[30];      // Para Guardar el ID del Dispositivo para el MQTT Broker
char mqttserver[30];  // Para Guardar el servidor del MQTT Broker

#define SW "SW20LP20CE07" //Software Version
#define HW "HW20LP20CE07" //Hardware Version

/****************************************
 * librerias control de version
 ****************************************/

