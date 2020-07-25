/********************************************************
 * CUBAELECTRONICA
 * Configuración con JSON e SPIFFS
 * Multiplataforma ESP32 e ESP8266
 * 07/2020 - Yamir Hidalgo Peña
 * admin@cubaelectronica.com
 * https://cubaelectronica.com
 * SW20LP20CE07
 */
//Librerias de funciones
#include <funciones.h>

// Requisitos Web --------------------------------------
void handleHome()
{
  // Home
  File file = SPIFFS.open(F("/Home.htm"), "r");
  if (file)
  {
    file.setTimeout(100);
    String s = file.readString();
    file.close();

    // Atualiza contenido dinamico
    s.replace(F("#id#"), id);
    s.replace(F("#led#"), ledOn ? F("ON") : F("OFF"));
    s.replace(F("#bootCount#"), String(bootCount));
#ifdef ESP8266
    s.replace(F("#serial#"), hexStr(ESP.getChipId()));
#else
    s.replace(F("#serial#"), hexStr(ESP.getEfuseMac()));
#endif
    s.replace(F("#software#"), softwareStr());
    s.replace(F("#sysIP#"), ipStr(WiFi.status() == WL_CONNECTED ? WiFi.localIP() : WiFi.softAPIP()));
    s.replace(F("#clientIP#"), ipStr(server.client().remoteIP()));
    s.replace(F("#active#"), longTimeStr(millis() / 1000));
    s.replace(F("#userAgent#"), server.header(F("User-Agent")));
    s.replace(F("#SW#"), F(SW));
    s.replace(F("#HW#"), F(HW));
    s.replace(F("#WFEstatus#"), WiFi.status() == WL_CONNECTED ? F("Online") : F("Offline"));
    s.replace(F("#WFSSID#"), WiFi.status() == WL_CONNECTED ? F(ssid) : F("--"));
    s.replace(F("#WFDBM#"), WiFi.status() == WL_CONNECTED ? String(WiFi.RSSI()) : F("0"));
    s.replace(F("#MQTTStatus#"), client.connected() ? F("Online") : F("Offline"));
    s.replace(F("#MQTTBroker#"), client.connected() ? F(mqttBroker) : F("--"));

    // Envia dados
    server.send(200, F("text/html"), s);
    log("Home - Cliente: " + ipStr(server.client().remoteIP()) +
        (server.uri() != "/" ? " [" + server.uri() + "]" : ""));
  }
  else
  {
    server.send(500, F("text/plain"), F("Home - ERROR 500"));
    log(F("Home - ERROR leyendo archivo"));
  }
}

void handleConfig()
{
  // Config
  File file = SPIFFS.open(F("/Config.htm"), "r");
  if (file)
  {
    file.setTimeout(100);
    String s = file.readString();
    file.close();

    // Atualiza el contenido dinamico
    s.replace(F("#id#"), id);
    s.replace(F("#ledOn#"), ledOn ? " checked" : "");
    s.replace(F("#ledOff#"), !ledOn ? " checked" : "");
    s.replace(F("#ssid#"), ssid);
    s.replace(F("#mqttuser#"), mqttuser);
    s.replace(F("#mqttserver#"), mqttserver);
    // Send data
    server.send(200, F("text/html"), s);
    log("Config - Cliente: " + ipStr(server.client().remoteIP()));
  }
  else
  {
    server.send(500, F("text/plain"), F("Config - ERROR 500"));
    log(F("Config - ERROR leyendo el archivo"));
  }
}

void handleConfigSave()
{
  // Graba Configuración desde Config
  // Verifica número de campos recebidos
#ifdef ESP8266
  // ESP8266 gera o campo adicional "plain" via post
  if (server.args() == 8)
  {
#else
  // ESP32 envia apenas 7 campos
  if (server.args() == 7)
  {
#endif
    String s;

    // Grava id
    s = server.arg("id");
    s.trim();
    if (s == "")
    {
      s = deviceID();
    }
    strlcpy(id, s.c_str(), sizeof(id));

    // Graba ssid
    s = server.arg("ssid");
    s.trim();
    strlcpy(ssid, s.c_str(), sizeof(ssid));

    // Graba pw
    s = server.arg("pw");
    s.trim();
    if (s != "")
    {
      // Actualiza contraseña
      strlcpy(pw, s.c_str(), sizeof(pw));
    }

    // Graba mqttuser
    s = server.arg("mqttuser");
    s.trim();
    if (s != "")
    {
      // Atualiza usuario mqtt
      strlcpy(mqttuser, s.c_str(), sizeof(mqttuser));
    }

    // Graba mqttpass
    s = server.arg("mqttpass");
    s.trim();
    if (s != "")
    {
      // Atualiza contraseña del user mqtt
      strlcpy(mqttpass, s.c_str(), sizeof(mqttpass));
    }

    // Graba mqttserver
    s = server.arg("mqttserver");
    s.trim();
    if (s != "")
    {
      // Atualiza server mqtt
      strlcpy(mqttserver, s.c_str(), sizeof(mqttserver));
    }

    // Graba ledOn
    ledOn = server.arg("led").toInt();

    // Atualiza LED
    ledSet();

    // Graba configuracion
    if (configSave())
    {
      server.send(200, F("text/html"), F("<html><meta charset='UTF-8'><script>alert('Configuración guardada.');history.back()</script></html>"));
      log("ConfigSave - Cliente: " + ipStr(server.client().remoteIP()));
    }
    else
    {
      server.send(200, F("text/html"), F("<html><meta charset='UTF-8'><script>alert('Falló la configuración.');history.back()</script></html>"));
      log(F("ConfigSave - ERROR salvando Configuración"));
    }
  }
  else
  {
    server.send(200, F("text/html"), F("<html><meta charset='UTF-8'><script>alert('Error de parámetros.');history.back()</script></html>"));
  }
}

void handleReconfig()
{
  // Reinicia Config
  configReset();

  // Atualiza LED
  ledSet();

  // Graba configuracion
  if (configSave())
  {
    server.send(200, F("text/html"), F("<html><meta charset='UTF-8'><script>alert('Configuración reiniciada.');window.location = '/'</script></html>"));
    log("Reconfigurar - Cliente: " + ipStr(server.client().remoteIP()));
  }
  else
  {
    server.send(200, F("text/html"), F("<html><meta charset='UTF-8'><script>alert('Falló el reinicio de la configuración.');history.back()</script></html>"));
    log(F("Reconfigurar - ERROR reinicio Configuración"));
  }
}

void handleReboot()
{
  // Reboot
  File file = SPIFFS.open(F("/Reboot.htm"), "r");
  if (file)
  {
    server.streamFile(file, F("text/html"));
    file.close();
    log("Reboot - Cliente: " + ipStr(server.client().remoteIP()));
    delay(100);
    ESP.restart();
  }
  else
  {
    server.send(500, F("text/plain"), F("Reboot - ERROR 500"));
    log(F("Reboot - ERROR leyendo archivo"));
  }
}

void handleCSS()
{
  // Arquivo CSS
  File file = SPIFFS.open(F("/assets/docs.min.css"), "r");
  if (file)
  {
    // Define cache para 3 dias
    server.sendHeader(F("Cache-Control"), F("public, max-age=172800"));
    server.streamFile(file, F("text/css"));
    file.close();
    log("CSS - Cliente: " + ipStr(server.client().remoteIP()));
  }
  else
  {
    server.send(500, F("text/plain"), F("CSS - ERROR 500"));
    log(F("CSS - ERROR leyendo archivo"));
  }
}

void handleICON()
{
  // Arquivo tft
  File file = SPIFFS.open(F("/assets/petalicon.ttf"), "r");
  if (file)
  {
    // Define cache para 3 dias
    server.sendHeader(F("Cache-Control"), F("public, max-age=172800"));
    server.streamFile(file, F("format('truetype')"));
    file.close();
    log("ICON - Cliente: " + ipStr(server.client().remoteIP()));
  }
  else
  {
    server.send(500, F("text/plain"), F("CSS - ERROR 500"));
    log(F("ICON - ERROR leyendo archivo"));
  }
}

void handleICON2()
{
  // Arquivo woff
  File file = SPIFFS.open(F("/assets/petalicon.woff"), "r");
  if (file)
  {
    // Define cache para 3 dias
    server.sendHeader(F("Cache-Control"), F("public, max-age=172800"));
    server.streamFile(file, F("format('woff')"));
    file.close();
    log("ICON2 - Cliente: " + ipStr(server.client().remoteIP()));
  }
  else
  {
    server.send(500, F("text/plain"), F("CSS - ERROR 500"));
    log(F("ICON2 - ERROR leyendo archivo"));
  }
}

void handleICON3()
{
  // Arquivo woff2
  File file = SPIFFS.open(F("/assets/petalicon.woff2"), "r");
  if (file)
  {
    // Define cache para 3 dias
    server.sendHeader(F("Cache-Control"), F("public, max-age=172800"));
    server.streamFile(file, F("format('woff2')"));
    file.close();
    log("ICON3 - Cliente: " + ipStr(server.client().remoteIP()));
  }
  else
  {
    server.send(500, F("text/plain"), F("CSS - ERROR 500"));
    log(F("ICON3 - ERROR leyendo archivo"));
  }
}

void handleLogo()
{
  // Arquivo PNG logo
  File file = SPIFFS.open(F("/assets/logo.png"), "r");
  if (file)
  {
    // Define cache para 3 dias
    server.sendHeader(F("Cache-Control"), F("public, max-age=172800"));
    server.streamFile(file, F("image/png"));
    file.close();
    log("LOGO - Cliente: " + ipStr(server.client().remoteIP()));
  }
  else
  {
    server.send(500, F("text/plain"), F("CSS - ERROR 500"));
    log(F("LOGO - ERROR leyendo archivo"));
  }
}


// Setup -------------------------------------------
void setup()
{
  //-------------------------------
  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(BUILTIN_LED, ledChannel);
  pinMode(BUILTIN_LED, OUTPUT);

  client.setServer(mqttserver, 1883);        //Nos conectamos al Broker, Servidor, Puerto.
  client.setCallback(callback);

  pinMode(RELAY1, OUTPUT);                   // Relay1 como Salida
  pinMode(RELAY2, OUTPUT);                   // Relay2 como Salida
  dht.begin();                               //Inicializamos el DHT11
  pinMode(alarma1.PIN, INPUT_PULLUP);        //Activar las resistencias PullUp
  attachInterrupt(alarma1.PIN, in1, CHANGE); //Activar la Interrupcion por cambio de estado
  lastReconnectAttempt = 0;                  //Inicializamos la variable en 0
  //-----------------------------------------------
#ifdef ESP8266
  // Velocidade para ESP8266
  Serial.begin(74880);
#else
  // Velocidade para ESP32
  Serial.begin(115200);
#endif

  // SPIFFS
  if (!SPIFFS.begin())
  {
    log(F("SPIFFS ERROR"));
    while (true)
      ;
  }

  // Lee la Configuración
  configRead();

  // Incrementa contador de reinicios
  bootCount++;

  // Guarda la Configuración
  configSave();

  // LED
  pinMode(LED_PIN, OUTPUT);
  ledSet();

// WiFi Access Point
#ifdef ESP8266
  // Configura WiFi para ESP8266
  WiFi.hostname(deviceID());
  WiFi.softAP(deviceID(), deviceID());
#else
  // Configura WiFi para ESP32
  WiFi.setHostname(deviceID().c_str());
  WiFi.softAP(deviceID().c_str(), "yamir1984");
#endif
  log("WiFi AP " + deviceID() + " - IP " + ipStr(WiFi.softAPIP()));

  // Habilita roteamento DNS
  dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
  dnsServer.start(DNSSERVER_PORT, "*", WiFi.softAPIP());

  // WiFi Station
  WiFi.begin(ssid, pw);
  log("Conectando WiFi " + String(ssid));
  byte b = 0;
  while (WiFi.status() != WL_CONNECTED && b < 60)
  {
    b++;
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    // WiFi Station conectado
    log("WiFi conectado (" + String(WiFi.RSSI()) + ") IP " + ipStr(WiFi.localIP()));
  }
  else
  {
    log(F("WiFi no conectado"));
  }

  // WebServer
  server.on(F("/config"), handleConfig);
  server.on(F("/configSave"), handleConfigSave);
  server.on(F("/reconfig"), handleReconfig);
  server.on(F("/reboot"), handleReboot);
  server.on(F("/assets/docs.min.css"), handleCSS);
  server.on(F("/assets/petalicon.ttf"), handleICON);
  server.on(F("/assets/petalicon.woff"), handleICON2);
  server.on(F("/assets/petalicon.woff2"), handleICON3);
  server.on(F("/assets/logo.png"), handleLogo);
  server.onNotFound(handleHome);
  server.collectHeaders(WEBSERVER_HEADER_KEYS, 1);
  server.begin();

  // Listo
  log(F("Listo"));
}

// Loop --------------------------------------------
void loop()
{
  // WatchDog ----------------------------------------
  yield();

  // DNS ---------------------------------------------
  dnsServer.processNextRequest();

  // Web ---------------------------------------------
  server.handleClient();

  // MQTT -------------------------------------------------
  if (!client.connected() && WiFi.status() == WL_CONNECTED)
  {
    long now = millis();
    if (now - lastReconnectAttempt > 60000) //intenta conectarse a cada un minuto.
    {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect())
      {
        lastReconnectAttempt = 0;
      }
    }
  }
  else
  {
    // Client connected
    client.loop();
  }

  long now = millis();

  //Camturamos el Evento de la Entrada y lo enviamos por MQTT.
  if (alarma1.active)
  {
    value1 = digitalRead(alarma1.PIN); // En la interrupcion capturamos el estado del PIN
    if (value1 == HIGH)
    {
      static uint32_t lastMillis = 0;
      if (millis() - lastMillis > 15000)
      {
        lastMillis = millis();
        Serial.println("Alarma Encendida 25");
        String to_send = "GPIO25ON";
        to_send.toCharArray(payload, 25);
        String topico_aux = device_id + "/digital";
        topico_aux.toCharArray(topico, 25);
        client.publish(topico, payload); //Publicar por MQTT
        alarma1.active = false;
      }
    }
    else
    {
      Serial.println("Alarma Apagada 25");
      String to_send = "GPIO25OFF";
      to_send.toCharArray(payload, 25);
      String topico_aux = device_id + "/digital";
      topico_aux.toCharArray(topico, 25);
      client.publish(topico, payload);
      alarma1.active = false;
    }
    delay(1000); //Antirrebote.
  }
  //Publicar la temperatura interna del dispositivo y del DHT11.
  if (now - lastMsg > 60000)
  {
    lastMsg = now;
    float temp = (temprature_sens_read() - 32) / 1.8;
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    float f = dht.readTemperature(true);
    float wifi = WiFi.RSSI(); // Capturamos el nivel de la señal WIFI
                              // Si hay fallo en las lecturas lo intenta de nuevo.
    if (isnan(h) || isnan(t) || isnan(f))
    {
      Serial.println("Falló al leer el sensor DHT !");
      return;
    }
    float hic = dht.computeHeatIndex(t, h, false);
    String ip = WiFi.localIP().toString(); //Capturamos el IP de la red local
    // Publicamos el Topic con los valores, RSSI, IP.
    String to_send = String(temp) + "," + String(t) + "," + String(wifi) + "," + String(h) + "," + String(hic) + "," + ip;
    to_send.toCharArray(payload, 50);
    String topico_aux = device_id + "/valores";
    topico_aux.toCharArray(topico, 50);
    client.publish(topico, payload);
  }

 }

 /*
 Medium quality:   50%      ->   -75dBm   = (50 / 2) - 100
Low quality:      -96dBm   ->   8%       = 2 * (-96 + 100)
quality = abs(RSSI)
% = 150 - (5/3) * quality
setCpuFrecuencyMhz();
getCpuFrecuencyMhz();
Get free heap
*/
