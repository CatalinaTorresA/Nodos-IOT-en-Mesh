#ifndef DB_CONNECTION_H
#define DB_CONNECTION_H

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <Arduino.h>

void dbConnection(int node, double temp, double hum, String datetime, String leds);

#endif // DB_CONNECTION_H