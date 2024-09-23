#include "DHT.h"
#include <Adafruit_Sensor.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>
#include <WiFi.h>
#include <time.h>
#include "esp_sleep.h" // Para deep sleep

// WiFi Credentials
const char *ssid = "DIEGO";
const char *wifipw = "10528152";

// Mesh Network Details
#define MESH_PREFIX "meshcbm123"
#define MESH_PASSWORD "cbmforever"
#define MESH_PORT 5555

// DHT Sensor Setup
#define DHTPIN 5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Node Number
int nodeNumber = 1;
bool ackReceived = false;            // Para verificar si se recibió confirmación del receptor
unsigned long waitForAckTime = 3000; // Tiempo máximo para esperar la confirmación (3 segundos)
unsigned long sendTime = 0;          // Tiempo en que se envió el mensaje

// Scheduler and Mesh Setup
Scheduler userScheduler;
painlessMesh mesh;

// Prototipos
void sendMessage();
void enterDeepSleep();
void receivedCallback(uint32_t from, String &msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void initTime(String timezone);
String getCurrentTime();

void startWifi()
{
  WiFi.begin(ssid, wifipw);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("WiFi connected.");
}

void initTime(String timezone)
{
  struct tm timeinfo;
  configTime(0, 0, "pool.ntp.org");
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }
  setenv("TZ", timezone.c_str(), 1);
  tzset();
}

String getCurrentTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    return "00-00-0000 00:00:00";
  }
  char buffer[25];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buffer);
}

String getReadings()
{
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t))
  {
    Serial.println("Failed to read from DHT sensor!");
    return "";
  }

  // Preparar los datos en formato JSON
  JSONVar jsonReadings;
  jsonReadings["node"] = nodeNumber;
  jsonReadings["temp"] = t;
  jsonReadings["hum"] = h;
  jsonReadings["datetime"] = getCurrentTime(); // Agregar la fecha y hora

  String readings = JSON.stringify(jsonReadings);
  Serial.println("Data collected: " + readings);

  return readings;
}

void sendMessage()
{
  String msg = getReadings(); // Obtener los datos del sensor
  if (msg.length() > 0)
  {
    mesh.sendBroadcast(msg); // Enviar mensaje en la red mesh
    Serial.println("Message sent: " + msg);
    sendTime = millis(); // Registrar el tiempo en que se envió el mensaje
  }
  else
  {
    Serial.println("No valid data to send.");
  }
}

void enterDeepSleep()
{
  Serial.println("Entering deep sleep for 7 seconds...");
  esp_sleep_enable_timer_wakeup(7 * 1000000); // Dormir por 7 segundos
  esp_deep_sleep_start();
}

void receivedCallback(uint32_t from, String &msg)
{
  Serial.printf("Received from %u: %s\n", from, msg.c_str());

  if (msg == "ACK")
  {
    // Si se recibe una confirmación del receptor, entrar en sleep
    Serial.println("Acknowledgment received. Preparing to sleep.");
    ackReceived = true; // Confirmación recibida
    enterDeepSleep();   // Entrar en deep sleep por 7 segundos
  }
}

void newConnectionCallback(uint32_t nodeId)
{
  Serial.printf("New connection, nodeId = %u\n", nodeId);
  sendMessage(); // Enviar los datos al receptor al conectar
}

void changedConnectionCallback()
{
  Serial.println("Connections changed");
}

void setup()
{
  Serial.begin(115200);
  startWifi();
  initTime("<-05>5"); // Configurar la zona horaria (ajustar según sea necesario)
  dht.begin();        // Iniciar el sensor DHT
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);

  // Mantener despierto por 3 segundos para la conexión inicial
  Serial.println("Staying awake for 3 seconds to establish connection...");
  delay(3000);
  sendMessage(); // Enviar mensaje una vez conectado
}

void loop()
{
  mesh.update();

  // Si ya se envió el mensaje y no se ha recibido ACK, verificar timeout
  if (sendTime > 0 && !ackReceived)
  {
    if (millis() - sendTime >= waitForAckTime)
    {
      // Si no se recibió ACK en 3 segundos, entrar en sleep de todas maneras
      Serial.println("No acknowledgment received within timeout. Going to sleep.");
      enterDeepSleep();
    }
  }
}
