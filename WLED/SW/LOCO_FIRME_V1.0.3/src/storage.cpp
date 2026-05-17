// SW/LOCO_FIRME_V1.0.3/src/storage.cpp
#include "storage.h"
#include "config.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

bool storageInit() {
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed — formatting...");
    LittleFS.format();
    return LittleFS.begin();
  }
  return true;
}

bool loadWifiConfig(char* ssid, char* pass, size_t maxLen) {
  if (!LittleFS.exists(WIFI_CONFIG_FILE)) return false;
  File f = LittleFS.open(WIFI_CONFIG_FILE, "r");
  if (!f) return false;
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;
  strlcpy(ssid, doc["ssid"] | "", maxLen);
  strlcpy(pass, doc["pass"] | "", maxLen);
  return strlen(ssid) > 0;
}

bool saveWifiConfig(const char* ssid, const char* pass) {
  File f = LittleFS.open(WIFI_CONFIG_FILE, "w");
  if (!f) return false;
  StaticJsonDocument<256> doc;
  doc["ssid"] = ssid;
  doc["pass"] = pass;
  serializeJson(doc, f);
  f.close();
  return true;
}

void clearWifiConfig() {
  LittleFS.remove(WIFI_CONFIG_FILE);
  Serial.println("WiFi config cleared");
}
