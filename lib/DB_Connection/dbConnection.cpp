#include "dbConnection.h"

const char *ssid = "Wifi Name";
const char *password = "Wifi Password";
const char *serverName = "https://example.com/post-esp-data.php";
String apiKeyValue = "ApiKey";

void dbConnection(int node, double temp, double hum, String datetime, String leds)
{
    WiFi.begin(ssid, password);
    Serial.println("Connecting");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(250);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());

    // Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED)
    {
        WiFiClientSecure *client = new WiFiClientSecure;
        client->setInsecure();
        HTTPClient https;

        https.begin(*client, serverName);
        https.addHeader("Content-Type", "application/x-www-form-urlencoded");

        // Preparation of the HTTP POST request data
        String httpRequestData = "api_key=" + apiKeyValue + "&node=" + String(node) + "&temperature=" + String(temp) + "&humidity=" + String(hum) + "&datetime=" + datetime + "&leds=" + leds + "";

        Serial.print("httpRequestData: ");
        Serial.println(httpRequestData);

        // Send HTTP POST request
        int httpResponseCode = https.POST(httpRequestData);

        if (httpResponseCode > 0)
        {
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);
        }
        else
        {
            Serial.print("Error code: ");
            Serial.println(httpResponseCode);
        }

        https.end();
    }
    else
    {
        Serial.println("WiFi Disconnected");
    }

    WiFi.disconnect();
}
