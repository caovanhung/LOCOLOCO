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

String getDeviceId() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char id[20];
  // Use last 4 bytes of MAC for a compact unique ID
  snprintf(id, sizeof(id), "esp_%02x%02x%02x%02x", mac[2], mac[3], mac[4], mac[5]);
  return String(id);
}

String getMacStr() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char s[18];
  snprintf(s, sizeof(s), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(s);
}
