// SW/LOCO_FIRME_V1.0.3/src/main.cpp
#include <Arduino.h>
#include "config.h"
#include "led_control.h"
#include "network.h"
#include "mqtt_handler.h"
#include "sensor.h"

// Callback khi nhận lệnh LED từ MQTT
static void onLedCommand(const char* dp1, int dp2, int dp3, const char* dp4) {
  bool changed = applyDPs(dp1, dp2, dp3, dp4);
  if (changed) {
    LedState s = getLedState();
    mqttPublishState(s.on, s.bri, s.effect, s.r, s.g, s.b);
  }
}

// Callback khi sensor đọc xong
static void onSensorRead(int pir, int ldr, float temp, float hum,
                         bool hasPir, bool hasLdr, bool hasDht) {
  mqttPublishSensor(pir, ldr, temp, hum, hasPir, hasLdr, hasDht);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nLOCOLOCO Firmware V1.0.3");

  ledInit();
  sensorInit();
  networkInit();

  if (networkConnected()) {
    mqttInit(onLedCommand);
  }
}

void loop() {
  networkLoop();
  if (networkConnected()) {
    mqttLoop();
    sensorLoop(onSensorRead);
  }
  ledLoop();
}
