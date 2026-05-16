// SW/LOCO_FIRME_V1.0.3/src/sensor.h
#pragma once
#include <Arduino.h>
#include <functional>

using SensorPublishFn = std::function<void(int pir, int ldr, float temp, float hum,
                                           bool hasPir, bool hasLdr, bool hasDht)>;

void sensorInit();
void sensorLoop(SensorPublishFn publishFn);
