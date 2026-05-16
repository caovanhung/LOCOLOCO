// SW/LOCO_FIRME_V1.0.3/src/mqtt_handler.h
#pragma once
#include <Arduino.h>
#include <functional>

using CommandCallback = std::function<void(const char* dp1, int dp2, int dp3, const char* dp4)>;

void mqttInit(CommandCallback onCmd);
void mqttLoop();
bool mqttConnected();
void mqttPublishState(bool on, uint8_t bri, uint8_t effect, uint8_t r, uint8_t g, uint8_t b);
void mqttPublishSensor(int pirVal, int ldrVal, float temp, float hum,
                       bool hasPir, bool hasLdr, bool hasDht);
