// SW/LOCO_FIRME_V1.0.3/src/config.h
#pragma once

// === WiFi (stored in flash — set via AP setup page, not hardcoded) ===
#define WIFI_CONFIG_FILE  "/wifi.json"   // LittleFS path
#define WIFI_TIMEOUT_MS   20000          // 20s to connect before AP fallback

// === AP Setup mode ===
// Press RESET, then immediately hold FLASH button (GPIO0/D3) for 3 seconds
// Device will broadcast "LOCOLOCO-XXXX" open network
#define AP_SSID_PREFIX    "LOCOLOCO"
#define AP_PASS           ""             // open network (no password)
#define SETUP_PIN         0              // GPIO0 / D3 / FLASH button (active LOW)
#define SETUP_HOLD_MS     3000           // hold duration to trigger AP mode

// === MQTT Broker ===
#define MQTT_HOST         "hung-test.codeaplha.biz"
#define MQTT_PORT         1883
#define MQTT_USER         "device"       // shared user for all ESP devices (must match Mosquitto passwd)
#define MQTT_PASS         "Loco@device2024"  // must match password set in deploy/mosquitto/passwd
#define MQTT_KEEPALIVE    120

// Topics are built dynamically in mqtt_handler.cpp using MAC-based device ID

// === LED ===
#define LED_PIN           2              // GPIO2 / D4
#define LED_COUNT         30
#define LED_BRIGHTNESS    200

// === Sensor enable flags ===
#define ENABLE_PIR        1
#define ENABLE_LDR        0
#define ENABLE_DHT        0

// === Sensor pins ===
#define PIR_PIN           5              // GPIO5 / D1
#define LDR_PIN           A0
#define DHT_PIN           4              // GPIO4 / D2
#define DHT_TYPE          DHT11

// === Sensor publish intervals (ms) ===
#define PIR_INTERVAL_MS   500
#define LDR_INTERVAL_MS   5000
#define DHT_INTERVAL_MS   30000

// === Protocol ===
#define PROTO_VERSION     1
