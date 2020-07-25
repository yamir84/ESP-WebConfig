//Librerias de configuraciones
#include <config.h>

/****************************************
 * Sensor Interno CPU
 ****************************************/
#ifdef ESP8266
    //Para ESP8266 no tiene temperatura de CPU
#else
    //Para ESP32 temperatura del CPU
    #ifdef __cplusplus
    extern "C"
    {
    #endif
    uint8_t temprature_sens_read();
    #ifdef __cplusplus
    }
    #endif
    uint8_t temprature_sens_read();
#endif

/****************************************
 * IRAM_ATTR In1
 ****************************************/
void IRAM_ATTR in1()
{
    alarma1.active = true;
}
/****************************************
 * Fin de las Funciones
 ****************************************/

// Funções Genéricas ------------------------------------
void log(String s)
{
    // Gera log na Serial
    Serial.println(s);
}

String softwareStr()
{
    // Retorna nome do software
    return String(__FILE__).substring(String(__FILE__).lastIndexOf("\\") + 1);
}

String longTimeStr(const time_t &t)
{
    // Retorna segundos como "d:hh:mm:ss"
    String s = String(t / SECS_PER_DAY) + ':';
    if (hour(t) < 10)
    {
        s += '0';
    }
    s += String(hour(t)) + ':';
    if (minute(t) < 10)
    {
        s += '0';
    }
    s += String(minute(t)) + ':';
    if (second(t) < 10)
    {
        s += '0';
    }
    s += String(second(t));
    return s;
}

String hexStr(const unsigned long &h, const byte &l = 8)
{
    // Retorna valor em formato hexadecimal
    String s;
    s = String(h, HEX);
    s.toUpperCase();
    s = ("00000000" + s).substring(s.length() + 8 - l);
    return s;
}

String deviceID()
{
// Retorna ID padrão
#ifdef ESP8266
    // ESP8266 utiliza função getChipId()
    return "CE-" + hexStr(ESP.getChipId());
#else
    // ESP32 utiliza função getEfuseMac()
    return "CE-" + hexStr(ESP.getEfuseMac());
#endif
}

String ipStr(const IPAddress &ip)
{
    // Retorna IPAddress em formato "n.n.n.n"
    String sFn = "";
    for (byte bFn = 0; bFn < 3; bFn++)
    {
        sFn += String((ip >> (8 * bFn)) & 0xFF) + ".";
    }
    sFn += String(((ip >> 8 * 3)) & 0xFF);
    return sFn;
}

void ledSet()
{
    // Define estado do LED
    digitalWrite(LED_PIN, ledOn ? LED_ON : LED_OFF);
}

// Funciones de Configuración ------------------------------
void configReset()
{
    // Define configuración padron
    strlcpy(id, "CE ESP_Device", sizeof(id));
    strlcpy(ssid, "", sizeof(ssid));
    strlcpy(pw, "", sizeof(pw));
    ledOn = false;
    bootCount = 0;
    strlcpy(mqttuser, "curso_iot", sizeof(mqttuser));
    strlcpy(mqttpass, "cubaelectronica", sizeof(mqttpass));
    strlcpy(mqttserver, "cubaelectronica.com", sizeof(mqttserver));
}

boolean configRead()
{
    // Lê configuração
    StaticJsonDocument<JSON_SIZE> jsonConfig;

    File file = SPIFFS.open(F("/Config.json"), "r");
    if (deserializeJson(jsonConfig, file))
    {
        // Si falla la lectura asume valores padrones
        configReset();

        log(F("Falló la lectura de CONFIG, asumiendo valores padrones."));
        return false;
    }
    else
    {
        // Si lee el archivo
        strlcpy(id, jsonConfig["id"] | "", sizeof(id));
        strlcpy(ssid, jsonConfig["ssid"] | "", sizeof(ssid));
        strlcpy(pw, jsonConfig["pw"] | "", sizeof(pw));
        ledOn = jsonConfig["led"] | false;
        bootCount = jsonConfig["boot"] | 0;
        strlcpy(mqttuser, jsonConfig["mqttuser"] | "", sizeof(mqttuser));
        strlcpy(mqttpass, jsonConfig["mqttpass"] | "", sizeof(mqttpass));
        strlcpy(mqttserver, jsonConfig["mqttserver"] | "", sizeof(mqttserver));

        file.close();

        log(F("\nLeyendo config:"));
        serializeJsonPretty(jsonConfig, Serial);
        log("");

        return true;
    }
}

boolean configSave()
{
    // Graba configuración
    StaticJsonDocument<JSON_SIZE> jsonConfig;

    File file = SPIFFS.open(F("/Config.json"), "w+");
    if (file)
    {
        // Atribulle valores al JSON y graba
        jsonConfig["id"] = id;
        jsonConfig["led"] = ledOn;
        jsonConfig["boot"] = bootCount;
        jsonConfig["ssid"] = ssid;
        jsonConfig["pw"] = pw;
        jsonConfig["mqttuser"] = mqttuser;
        jsonConfig["mqttpass"] = mqttpass;
        jsonConfig["mqttserver"] = mqttserver;

        serializeJsonPretty(jsonConfig, file);
        file.close();

        log(F("\nGrabando config:"));
        serializeJsonPretty(jsonConfig, Serial);
        log("");

        return true;
    }
    return false;
}

/****************************************
 * Funciones Auxiliares MQTT
 ****************************************/
WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE); //Pasamos el DHT pin y el DHT modelo a dht
/****************************************
 * Funsion recibir mensajes PubSubClient
 ****************************************/
void callback(char *topic, byte *payload, unsigned int length)
{
    String mensaje = "";
    Serial.print("Topico --> ");
    Serial.println(topic);

    for (int i = 0; i < length; i++)
    {
        mensaje += (char)payload[i];
    }

    mensaje.trim();
    Serial.println("Mensaje --> " + mensaje);
    String str_topic(topic);

    if (str_topic == device_id + "/command")
    {
        if (mensaje == "on")
        {
            digitalWrite(RELAY1, HIGH);
            //////////////Respondemos OK///////////
            String topico_aux = device_id + "/respuesta";
            topico_aux.toCharArray(topico, 25);
            client.publish(topico, "on_ok"); //Publicar respuesta on ok por MQTT
                                             ///////////////////////////////////////
        }
        else if (mensaje == "off")
        {
            digitalWrite(RELAY1, LOW);
            ///////////////Respondemos OK//////////
            String topico_aux = device_id + "/respuesta";
            topico_aux.toCharArray(topico, 25);
            client.publish(topico, "off_ok"); //Publicar respuesta off ok por MQTT
                                              ///////////////////////////////////////
        }
        else if (mensaje == "on1")
        {
            digitalWrite(RELAY2, HIGH);
            ///////////////Respondemos OK//////////
            String topico_aux = device_id + "/respuesta";
            topico_aux.toCharArray(topico, 25);
            client.publish(topico, "on1_ok"); //Publicar respuesta off ok por MQTT
                                              ///////////////////////////////////////
        }
        else if (mensaje == "off1")
        {
            digitalWrite(RELAY2, LOW);
            ///////////////Respondemos OK//////////
            String topico_aux = device_id + "/respuesta";
            topico_aux.toCharArray(topico, 25);
            client.publish(topico, "off1_ok"); //Publicar respuesta off ok por MQTT
                                               ///////////////////////////////////////
        }
    }

    if (str_topic == device_id + "/dimmer")
    {
        dimmer = 0;
        String text = mensaje;
        long i;
        i = text.toInt();
        dimmer = i;
        ledcWrite(ledChannel, dimmer * 2.55); // multiplicamos por 2.55*100 para llegar a 255 que seria el maximo a 8bit = 3.3V
        delay(7);
    }
}
/****************************************
 * Funsion reconectar de PubSubClient
 ****************************************/
const char* ID = strcpy(buffer, deviceID().c_str()); //Asigno el ID del dispositivo al ID para el broker 
const char* user = mqttuser;                         //Asigno el User del archivo JSON para el User del Broker
const char *password = mqttpass;                     //Asigno el Password del archivo JSON para el pasword del User del Broker

boolean reconnect()
{
    Serial.println("Intentando conexión al Broker MQTT...");
    // Conexion al Servidor MQTT , ClienteID, Usuario, Password.
    // Ver documentación => https://pubsubclient.knolleary.net/api.html
    if (client.connect(ID, user, password))
    {
        Serial.println("Conectado! al Broker MQTT cubaElectronica.com");
        // Nos suscribimos a comandos
        String topico_serial = device_id + "/command";
        topico_serial.toCharArray(topico, 25);
        client.subscribe(topico);
        // Nos suscribimos a dimmer
        String topico_serial2 = device_id + "/dimmer";
        topico_serial2.toCharArray(topico2, 25);
        client.subscribe(topico2);
    }
    return client.connected();
}

