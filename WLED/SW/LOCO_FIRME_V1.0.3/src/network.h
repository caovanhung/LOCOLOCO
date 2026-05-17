// SW/LOCO_FIRME_V1.0.3/src/network.h
#pragma once
#include <Arduino.h>

void networkInit();
bool networkConnected();
void networkLoop();

// Option A: MAC-based unique device ID (e.g. "esp_a1b2c3d4")
String getDeviceId();
// Full MAC string (e.g. "AA:BB:CC:DD:A1:B2")
String getMacStr();
