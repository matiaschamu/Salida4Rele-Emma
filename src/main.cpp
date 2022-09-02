#include <Arduino.h>
#include <ESP8266WiFi.h>
//#include <WiFi.h>
#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>

//#define debug

void setup_wifi();
void setup_MQTT();
void MQTT_Callback(char *, byte *, unsigned int);
void WebServer_Loop();
void Relay_Loop();
void DuckDNS_Loop();
void HealthChecks_Loop();
void HttpGet(String);
void MQTT_Reconnect();
String MqttStatus();
void SerialPrint();
void SerialPrint(String);
void SerialPrint(char);
void SerialPrint(int);
String convertToString(char *, int);
String convertToString(byte *, int);

#pragma region "Configuracion" //////////////////////////////////////////////////////////

const String Version = "1.0.5";
const char *ssid = "Camaras";
const char *password = "37615097";

IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(192, 168, 1, 1);
// IPAddress secondaryDNS(8, 8, 4, 4);

const char *mqtt_server = "192.168.1.10";
const char *mqtt_user = "matias";
const char *mqtt_pass = "Mato19428426.";

String urlDuckDNS = "http://www.duckdns.org/update/casaemmadebo/f4be5f35-a9c4-4837-b709-f38afbfaaabd";
String urlHealthChecks = "http://hc-ping.com/6b750dde-84ed-424a-b708-7c869b8c5253";

//Placa1
//#define Report_IP_DuckDNS
//#define Report_HealthChecks
// IPAddress local_IP(192, 168, 1, 62);
// const char *hostName = "Living";
// String Relay1_Name = "Luz Living";
// String Relay1_MQTT_Command = "Acantilados/Luz/Living/Comando";
// String Relay1_MQTT_Status = "Acantilados/Luz/Living/Estado";
// String Relay2_Name = "Luz Dormitorio";
// String Relay2_MQTT_Command = "Acantilados/Luz/Dormitorio/Comando";
// String Relay2_MQTT_Status = "Acantilados/Luz/Dormitorio/Estado";
// String Relay3_Name = "Luz Dorm Superior";
// String Relay3_MQTT_Command = "Acantilados/Luz/Superior/Comando";
// String Relay3_MQTT_Status = "Acantilados/Luz/Superior/Estado";
// String Relay4_Name = "Luz Garage";
// String Relay4_MQTT_Command = "Acantilados/Luz/Garage/Comando";
// String Relay4_MQTT_Status = "Acantilados/Luz/Garage/Estado";

//Placa2
#define Report_IP_DuckDNS
//#define Report_HealthChecks
IPAddress local_IP(192, 168, 1, 63);
const char *hostName = "Lavadero";
String Relay1_Name = "Luz Cocina";
String Relay1_MQTT_Command = "Acantilados/Luz/Cocina/Comando";
String Relay1_MQTT_Status = "Acantilados/Luz/Cocina/Estado";
String Relay2_Name = "Luz Lavadero";
String Relay2_MQTT_Command = "Acantilados/Luz/Lavadero/Comando";
String Relay2_MQTT_Status = "Acantilados/Luz/Lavadero/Estado";
String Relay3_Name = "Luz Farolas";
String Relay3_MQTT_Command = "Acantilados/Luz/Farolas/Comando";
String Relay3_MQTT_Status = "Acantilados/Luz/Farolas/Estado";
String Relay4_Name = "Bomba Agua";
String Relay4_MQTT_Command = "Acantilados/Luz/Bomba/Comando";
String Relay4_MQTT_Status = "Acantilados/Luz/Bomba/Estado";

#pragma endregion /////////////////////////////////////////////////////////////////////

#pragma region "Inicializacion de variables" //////////////////////////////////////////////////////////
bool Relay1Status = false;
bool Relay2Status = false;
bool Relay3Status = false;
bool Relay4Status = false;

byte Relay1_OFF[] = {0xa0, 0x01, 0x00, 0xa1};
byte Relay1_ON[] = {0xa0, 0x01, 0x01, 0xa2};
byte Relay2_OFF[] = {0xa0, 0x02, 0x00, 0xa2};
byte Relay2_ON[] = {0xa0, 0x02, 0x01, 0xa3};
byte Relay3_OFF[] = {0xa0, 0x03, 0x00, 0xa3};
byte Relay3_ON[] = {0xa0, 0x03, 0x01, 0xa4};
byte Relay4_OFF[] = {0xa0, 0x04, 0x00, 0xa4};
byte Relay4_ON[] = {0xa0, 0x04, 0x01, 0xa5};

int status = WL_IDLE_STATUS;
WiFiClient espClient;
PubSubClient client_MQTT(espClient);
long lastMsg5seg = 0;
long lastMsg1min = 0;
long lastMsg5min = 0;
char msg[50];

#pragma region "web server Config"
WiFiServer WEB_Server(8084);
String header;
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;
#pragma endregion
#pragma endregion /////////////////////////////////////////////////////////////////////

void setup()
{
  Serial.begin(115200);
  // pinMode(LED_BUILTIN, OUTPUT); // Initialize the BUILTIN_LED pin as an output
  SerialPrint("Configurando WiFI");
  setup_wifi();
  SerialPrint("Configurando MQTT");
  setup_MQTT();

  WEB_Server.begin();
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    setup_wifi();
  }

  client_MQTT.loop();
  WebServer_Loop();

  long now = millis();
  if (now - lastMsg5seg > 5000)
  {
    lastMsg5seg = now;
    Relay_Loop();
    if (!client_MQTT.loop())
    {
      MQTT_Reconnect();
    }
  }

  if (now - lastMsg1min > 60000)
  {
    lastMsg1min = now;
#ifdef Report_IP_DuckDNS
    HttpGet(urlDuckDNS);
#endif
  }

  if (now - lastMsg5min > 300000)
  {
    lastMsg5min = now;
#ifdef Report_HealthChecks
    HttpGet(urlHealthChecks);
#endif
  }
}

void setup_wifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(500);

  if (WiFi.config(local_IP, gateway, subnet, primaryDNS) == false)
  {
    SerialPrint("Configuration failed.");
  }

  SerialPrint();
  SerialPrint("Connecting to ");
  SerialPrint(ssid);
  WiFi.hostname(hostName);
  SerialPrint("WIFI Status  : ");
  SerialPrint(WiFi.status());
  SerialPrint("Conectando...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    SerialPrint(".");
    delay(500);
  }
  delay(500);
  SerialPrint("");
  SerialPrint("WiFi connected");
  SerialPrint("IP address: ");
  SerialPrint(WiFi.localIP().toString());

  SerialPrint("RRSI: ");
  SerialPrint(WiFi.RSSI());
}

void setup_MQTT()
{
  client_MQTT.setServer(mqtt_server, 1883);
  SerialPrint("Declarando el Callback");
  client_MQTT.setCallback(MQTT_Callback);
  SerialPrint("Setup done");
}

void MQTT_Reconnect()
{
  SerialPrint("Intentando MQTT Conexion...");
  SerialPrint(client_MQTT.state());

  if (client_MQTT.connect(hostName, mqtt_user, mqtt_pass))
  {
    SerialPrint("Conectado");
    if (Relay1_MQTT_Command != "")
    {
      SerialPrint("Suscribiendo en " + Relay1_MQTT_Command + " ...");
      while (client_MQTT.subscribe(Relay1_MQTT_Command.c_str()) == false)
      {
      }
      SerialPrint("Suscripcion OK");
    }

    if (Relay2_MQTT_Command != "")
    {
      SerialPrint("Suscribiendo en " + Relay2_MQTT_Command + " ...");
      while (client_MQTT.subscribe(Relay2_MQTT_Command.c_str()) == false)
      {
      }
      SerialPrint("Suscripcion OK");
    }

    if (Relay3_MQTT_Command != "")
    {
      SerialPrint("Suscribiendo en " + Relay3_MQTT_Command + " ...");
      while (client_MQTT.subscribe(Relay3_MQTT_Command.c_str()) == false)
      {
      }
      SerialPrint("Suscripcion OK");
    }

    if (Relay4_MQTT_Command != "")
    {
      SerialPrint("Suscribiendo en " + Relay4_MQTT_Command + " ...");
      while (client_MQTT.subscribe(Relay4_MQTT_Command.c_str()) == false)
      {
      }
      SerialPrint("Suscripcion OK");
    }
  }
  else
  {
    SerialPrint("Fallo conexion MQTT");
    SerialPrint(client_MQTT.state());
    SerialPrint("Intentando en 5 seg...");
  }
}

void MQTT_Callback(char *topic, byte *payload, unsigned int length)
{
  SerialPrint("Message arrived: ");
  String mTopic = convertToString(topic, strlen(topic));
  String mPayload = convertToString(payload, sizeof(payload));
  SerialPrint(mTopic);
  SerialPrint(mPayload);
  SerialPrint();

  if (mTopic == Relay1_MQTT_Command)
  {
    if ((char)payload[1] == 'N')
    {
      Relay1Status = true;
      SerialPrint("Relay1_ON");
    }
    else
    {
      Relay1Status = false;
      SerialPrint("Relay1_OFF");
    }
    lastMsg5seg = 0;
  }
  else if (mTopic == Relay2_MQTT_Command)
  {
    if ((char)payload[1] == 'N')
    {
      Relay2Status = true;
      SerialPrint("Relay2_ON");
    }
    else
    {
      Relay2Status = false;
      SerialPrint("Relay2_OFF");
    }
    lastMsg5seg = 0;
  }
  else if (mTopic == Relay3_MQTT_Command)
  {
    if ((char)payload[1] == 'N')
    {
      Relay3Status = true;
      SerialPrint("Relay3_ON");
    }
    else
    {
      Relay3Status = false;
      SerialPrint("Relay3_OFF");
    }
    lastMsg5seg = 0;
  }
  else if (mTopic == Relay4_MQTT_Command)
  {
    if ((char)payload[1] == 'N')
    {
      Relay4Status = true;
      SerialPrint("Relay4_ON");
    }
    else
    {
      Relay4Status = false;
      SerialPrint("Relay4_OFF");
    }
    lastMsg5seg = 0;
  }
}

void WebServer_Loop()
{
  WiFiClient client = WEB_Server.available();

  if (client)
  {
    currentTime = millis();
    previousTime = currentTime;
    bool reset = false;
    SerialPrint("New Client.");
    String currentLine = "";
    while (client.connected() && currentTime - previousTime <= timeoutTime)
    {
      currentTime = millis();
      if (client.available())
      {
        char c = client.read();
#ifdef debug
        Serial.write(c);
#endif
        header += c;
        if (c == '\n')
        {

          if (currentLine.length() == 0)
          {

            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            if (header.indexOf("GET /relay1/on") >= 0)
            {
              Relay1Status = 1;
            }
            else if (header.indexOf("GET /relay1/off") >= 0)
            {
              Relay1Status = 0;
            }
            else if (header.indexOf("GET /relay2/on") >= 0)
            {
              Relay2Status = 1;
            }
            else if (header.indexOf("GET /relay2/off") >= 0)
            {
              Relay2Status = 0;
            }
            else if (header.indexOf("GET /relay3/on") >= 0)
            {
              Relay3Status = 1;
            }
            else if (header.indexOf("GET /relay3/off") >= 0)
            {
              Relay3Status = 0;
            }
            else if (header.indexOf("GET /relay4/on") >= 0)
            {
              Relay4Status = 1;
            }
            else if (header.indexOf("GET /relay4/off") >= 0)
            {
              Relay4Status = 0;
            }
            else if (header.indexOf("GET /reset") >= 0)
            {
              reset = true;
            }
            lastMsg5seg = 0;

            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");

            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #555555; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #4CAF50;}");
            client.println(".button3 {background-color: #FF0000; font-size: 10px; padding: 10px 20px;}</style></head>");
            client.println("<body>");
            if (reset == false)
            {
              client.println("<h1>" + String(hostName) + " Web Server (" + WiFi.localIP().toString() + ")</h1>");
              client.println("<p> Version: " + Version + "</p>");

              if (Relay1Status == 0)
              {
                client.println("<p>Relay 1 estado: " + String(Relay1Status) + " --> " + Relay1_Name + "</p>");
                client.println("<p><a href=\"/relay1/on\"><button class=\"button\">OFF</button></a></p>");
              }
              else
              {
                client.println("<b style="
                               "color:red;"
                               ">Relay 1 estado: " +
                               String(Relay1Status) + " --> " + Relay1_Name + "</b>");
                client.println("<p><a href=\"/relay1/off\"><button class=\"button button2\">ON</button></a></p>");
              }

              if (Relay2Status == 0)
              {
                client.println("<p>Relay 2 estado: " + String(Relay2Status) + " --> " + Relay2_Name + "</p>");
                client.println("<p><a href=\"/relay2/on\"><button class=\"button\">OFF</button></a></p>");
              }
              else
              {
                client.println("<b style="
                               "color:red;"
                               ">Relay 2 estado: " +
                               String(Relay2Status) + " --> " + Relay2_Name + "</b>");
                client.println("<p><a href=\"/relay2/off\"><button class=\"button button2\">ON</button></a></p>");
              }

              if (Relay3Status == 0)
              {
                client.println("<p>Relay 3 estado: " + String(Relay3Status) + " --> " + Relay3_Name + "</p>");
                client.println("<p><a href=\"/relay3/on\"><button class=\"button\">OFF</button></a></p>");
              }
              else
              {
                client.println("<b style="
                               "color:red;"
                               ">Relay 3 estado: " +
                               String(Relay3Status) + " --> " + Relay3_Name + "</b>");
                client.println("<p><a href=\"/relay3/off\"><button class=\"button button2\">ON</button></a></p>");
              }

              if (Relay4Status == 0)
              {
                client.println("<p>Relay 4 estado: " + String(Relay4Status) + " --> " + Relay4_Name + "</p>");
                client.println("<p><a href=\"/relay4/on\"><button class=\"button\">OFF</button></a></p>");
              }
              else
              {
                client.println("<b style="
                               "color:red;"
                               ">Relay 4 estado: " +
                               String(Relay4Status) + " --> " + Relay4_Name + "</b>");
                client.println("<p><a href=\"/relay4/off\"><button class=\"button button2\">ON</button></a></p>");
              }

              client.println("<p>MQTT server status: " + MqttStatus() + "</p>");

#ifdef Report_IP_DuckDNS
              client.println("<p>DuckDNS Updated every 1 min: " + urlDuckDNS + "</p>");
#endif
#ifndef Report_IP_DuckDNS
              client.println("<p>DuckDNS Updated disabled</p>");
#endif
#ifdef Report_HealthChecks
              client.println("<p>HealthChecks Updated every 5 min: " + urlHealthChecks + "</p>");
#endif
#ifndef Report_HealthChecks
              client.println("<p>HealthChecks Updated disabled</p>");
#endif
              client.println("<p><a href=\"/reset\"><button class=\"button button3\">RESET</button></a></p>");
            }
            else
            {
              client.println("<script>");
              client.println("setTimeout(function(){");
              client.println("window.history.back();");
              client.println("}, 20000);");
              client.println("</script>");
              client.println("<p>Reseting in 5 sec... Reloading page in 20 sec</p>");
            }

            client.println("</body></html>");
            client.println();
            break;
          }
          else
          {
            currentLine = "";
          }
        }
        else if (c != '\r')
        {
          currentLine += c;
        }
      }
    }
    header = "";

    client.stop();
    SerialPrint("Client disconnected.");
    SerialPrint("");

    if (reset == true)
    {
      Serial.write(Relay1_OFF, sizeof(Relay1_OFF));
      delay(50);
      Serial.write(Relay2_OFF, sizeof(Relay2_OFF));
      delay(50);
      Serial.write(Relay3_OFF, sizeof(Relay3_OFF));
      delay(50);
      Serial.write(Relay4_OFF, sizeof(Relay4_OFF));
      delay(50);
      Serial.println(" ");
      Serial.println("Reset in 5 sec..");
      delay(5000);
      ESP.restart();
    }
  }
}

void Relay_Loop()
{
  if (Relay1_Name != "")
  {
    if (Relay1Status == true)
    {
      Serial.write(Relay1_ON, sizeof(Relay1_ON));
      client_MQTT.publish(Relay1_MQTT_Status.c_str(), "ON");
      SerialPrint("Relay 1 -> 1");
    }
    else
    {
      Serial.write(Relay1_OFF, sizeof(Relay1_OFF));
      client_MQTT.publish(Relay1_MQTT_Status.c_str(), "OFF");
      SerialPrint("Relay 1 -> 0");
    }
  }
  delay(50);
  if (Relay2_Name != "")
  {
    if (Relay2Status == true)
    {
      Serial.write(Relay2_ON, sizeof(Relay2_ON));
      client_MQTT.publish(Relay2_MQTT_Status.c_str(), "ON");
      SerialPrint("Relay 2 -> 1");
    }
    else
    {
      Serial.write(Relay2_OFF, sizeof(Relay2_OFF));
      client_MQTT.publish(Relay2_MQTT_Status.c_str(), "OFF");
      SerialPrint("Relay 2 -> 0");
    }
  }
  delay(50);
  if (Relay3_Name != "")
  {
    if (Relay3Status == true)
    {
      Serial.write(Relay3_ON, sizeof(Relay3_ON));
      client_MQTT.publish(Relay3_MQTT_Status.c_str(), "ON");
      SerialPrint("Relay 3 -> 1");
    }
    else
    {
      Serial.write(Relay3_OFF, sizeof(Relay3_OFF));
      client_MQTT.publish(Relay3_MQTT_Status.c_str(), "OFF");
      SerialPrint("Relay 3 -> 0");
    }
  }
  delay(50);
  if (Relay4_Name != "")
  {
    if (Relay4Status == true)
    {
      Serial.write(Relay4_ON, sizeof(Relay4_ON));
      client_MQTT.publish(Relay4_MQTT_Status.c_str(), "ON");
      SerialPrint("Relay 4 -> 1");
    }
    else
    {
      Serial.write(Relay4_OFF, sizeof(Relay4_OFF));
      client_MQTT.publish(Relay4_MQTT_Status.c_str(), "OFF");
      SerialPrint("Relay 4 -> 0");
    }
  }
}

void HttpGet(String url)
{
  if ((WiFi.status() == WL_CONNECTED))
  {
    String urlRequest = url;
    WiFiClient client;
    HTTPClient http;

    SerialPrint("Iniciando cliete HTTP");

    if (http.begin(client, urlRequest))
    {
      SerialPrint(urlRequest);

      int httpCode = http.GET();

      if (httpCode > 0)
      {
        SerialPrint(httpCode);

        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
        {
          String payload = http.getString();
          SerialPrint(payload);
        }
      }
      else
      {
        SerialPrint("[HTTP] Error on HTTP request");
        SerialPrint("[HTTP] GET... failed, error: %s\n" + http.errorToString(httpCode));
      }
      http.end();
    }
    else
    {
      SerialPrint("[HTTP] Unable to connect\n");
    }
  }
}

String MqttStatus()
{
  switch (client_MQTT.state())
  {
  case -4:
    return "MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time";
    break;
  case -3:
    return "MQTT_CONNECTION_LOST - the network connection was broken";
    break;
  case -2:
    return "MQTT_CONNECT_FAILED - the network connection failed";
    break;
  case -1:
    return "MQTT_DISCONNECTED - the client is disconnected cleanly";
    break;
  case 0:
    return "MQTT_CONNECTED - the client is connected";
    break;
  case 1:
    return "MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT";
    break;
  case 2:
    return "MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the client identifier";
    break;
  case 3:
    return "MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection";
    break;
  case 4:
    return "MQTT_CONNECT_BAD_CREDENTIALS - the username/password were rejected";
    break;
  case 5:
    return "MQTT_CONNECT_UNAUTHORIZED - the client was not authorized to connect";
    break;
  default:
    return "";
    break;
  }
}

String convertToString(char *a, int size)
{
  int i;
  String s = "";
  for (i = 0; i < size; i++)
  {
    s = s + a[i];
  }
  return s;
}
String convertToString(byte *a, int size)
{
  int i;
  String s = "";
  for (i = 0; i < size; i++)
  {
    s = s + a[i];
  }
  return s;
}

void SerialPrint()
{
  int a = 0;
  a = a;
#ifdef debug
  Serial.println();
#endif
}
void SerialPrint(String msg)
{
  int a = 0;
  a = a;
#ifdef debug
  Serial.println(msg);
#endif
}
void SerialPrint(char msg)
{
  int a = 0;
  a = a;
#ifdef debug
  Serial.println(msg);
#endif
}
void SerialPrint(int msg)
{
  int a = 0;
  a = a;
#ifdef debug
  Serial.println(msg);
#endif
}
