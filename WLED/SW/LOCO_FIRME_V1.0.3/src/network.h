// SW/LOCO_FIRME_V1.0.3/src/network.h
#pragma once
#include <Arduino.h>

// Attempts to connect to saved WiFi credentials.
// If no config saved, WiFi fails, or FLASH button held 3s at boot:
//   → enters AP mode (serves setup page), blocks until user configures, then restarts.
void networkInit();

bool networkConnected();
void networkLoop();

// MAC-based unique device ID (e.g. "esp_a1b2c3d4")
String getDeviceId();
// Full MAC string (e.g. "AA:BB:CC:DD:A1:B2")
String getMacStr();
