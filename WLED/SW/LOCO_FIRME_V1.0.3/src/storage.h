// SW/LOCO_FIRME_V1.0.3/src/storage.h
#pragma once
#include <Arduino.h>

bool storageInit();
bool loadWifiConfig(char* ssid, char* pass, size_t maxLen);
bool saveWifiConfig(const char* ssid, const char* pass);
void clearWifiConfig();
