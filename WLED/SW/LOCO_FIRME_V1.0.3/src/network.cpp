// SW/LOCO_FIRME_V1.0.3/src/network.cpp
#include "network.h"
#include "config.h"
#include <ESP8266WiFi.h>

void networkInit() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting WiFi");
  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t < 15000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? "\nWiFi OK" : "\nWiFi FAIL");
}

bool networkConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void networkLoop() {
  static unsigned long lastRetry = 0;
  if (WiFi.status() != WL_CONNECTED && millis() - lastRetry >= 5000) {
    lastRetry = millis();
    WiFi.reconnect();
  }
}
