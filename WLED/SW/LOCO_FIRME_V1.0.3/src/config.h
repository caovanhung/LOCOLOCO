// SW/LOCO_FIRME_V1.0.3/src/config.h
#pragma once

// === WiFi ===
#define WIFI_SSID        "Loco"
#define WIFI_PASS        "matkhau1"

// === MQTT Broker (MQTTS port 8883) ===
#define MQTT_HOST        "your-vps.com"
#define MQTT_PORT        8883
// Shared MQTT user for all ESP devices — no per-device credentials needed
#define MQTT_USER        "device"
#define MQTT_PASS        "device_password_here"
#define MQTT_KEEPALIVE   120

// Topics are built dynamically in mqtt_handler.cpp using MAC-based device ID

// === LED ===
#define LED_PIN          2       // GPIO2 / D4
#define LED_COUNT        30
#define LED_BRIGHTNESS   200

// === Sensor enable flags ===
#define ENABLE_PIR       1
#define ENABLE_LDR       0
#define ENABLE_DHT       0

// === Sensor pins ===
#define PIR_PIN          5       // GPIO5 / D1
#define LDR_PIN          A0
#define DHT_PIN          4       // GPIO4 / D2
#define DHT_TYPE         DHT11

// === Sensor publish intervals (ms) ===
#define PIR_INTERVAL_MS  500
#define LDR_INTERVAL_MS  5000
#define DHT_INTERVAL_MS  30000

// === Protocol ===
#define PROTO_VERSION    1
