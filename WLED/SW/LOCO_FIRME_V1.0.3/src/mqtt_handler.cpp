// SW/LOCO_FIRME_V1.0.3/src/mqtt_handler.cpp
#include "mqtt_handler.h"
#include "config.h"
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

static WiFiClientSecure wifiClient;
static PubSubClient     mqttClient(wifiClient);
static CommandCallback  cmdCallback;
static char             msgIdBuf[9];

static void genMsgId() {
  uint32_t r = ESP.getCycleCount() ^ millis();
  snprintf(msgIdBuf, sizeof(msgIdBuf), "%08x", r);
}

static bool buildEnvelope(JsonDocument& doc, const char* type) {
  genMsgId();
  doc["v"]     = PROTO_VERSION;
  doc["msgId"] = msgIdBuf;
  doc["ts"]    = (long long)millis();
  doc["devId"] = DEVICE_ID;
  doc["type"]  = type;
  return true;
}

static void onMqttMessage(char* topic, byte* payload, unsigned int len) {
  char buf[512];
  if (len >= sizeof(buf)) return;
  memcpy(buf, payload, len);
  buf[len] = '\0';

  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, buf) != DeserializationError::Ok) return;
  if (!doc.containsKey("data") || !doc["data"].containsKey("dps")) return;

  JsonObject dps = doc["data"]["dps"];

  const char* dp1 = nullptr;
  int dp2 = -1, dp3 = -1;
  const char* dp4 = nullptr;
  char dp4buf[16] = {0};

  if (dps.containsKey("1")) {
    dp1 = dps["1"].as<bool>() ? "true" : "false";
  }
  if (dps.containsKey("2")) dp2 = dps["2"].as<int>();
  if (dps.containsKey("3")) dp3 = dps["3"].as<int>();
  if (dps.containsKey("4")) {
    strlcpy(dp4buf, dps["4"].as<const char*>(), sizeof(dp4buf));
    dp4 = dp4buf;
  }

  if (cmdCallback) cmdCallback(dp1, dp2, dp3, dp4);
}

static void mqttConnect() {
  char lwtPayload[128];
  StaticJsonDocument<128> lwtDoc;
  buildEnvelope(lwtDoc, "evt");
  lwtDoc["data"]["online"] = false;
  serializeJson(lwtDoc, lwtPayload, sizeof(lwtPayload));

  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(onMqttMessage);
  mqttClient.setKeepAlive(MQTT_KEEPALIVE);

  if (mqttClient.connect(DEVICE_ID, MQTT_USER, MQTT_PASS,
                         TOPIC_EVT_STATUS, 0, true, lwtPayload)) {
    mqttClient.subscribe(TOPIC_CMD_LED, 1);
    StaticJsonDocument<128> onlineDoc;
    buildEnvelope(onlineDoc, "evt");
    onlineDoc["data"]["online"] = true;
    char onlineBuf[128];
    serializeJson(onlineDoc, onlineBuf, sizeof(onlineBuf));
    mqttClient.publish(TOPIC_EVT_STATUS, onlineBuf, true);
    Serial.println("MQTT connected");
  }
}

void mqttInit(CommandCallback onCmd) {
  cmdCallback = onCmd;
  wifiClient.setInsecure();
  mqttConnect();
}

void mqttLoop() {
  if (!mqttClient.connected()) mqttConnect();
  mqttClient.loop();
}

bool mqttConnected() { return mqttClient.connected(); }

void mqttPublishState(bool on, uint8_t bri, uint8_t effect,
                      uint8_t r, uint8_t g, uint8_t b) {
  StaticJsonDocument<256> doc;
  buildEnvelope(doc, "rpt");
  JsonObject dps = doc["data"].createNestedObject("dps");
  dps["1"] = on;
  dps["2"] = bri;
  dps["3"] = effect;
  char colorBuf[16];
  snprintf(colorBuf, sizeof(colorBuf), "%d,%d,%d", r, g, b);
  dps["4"] = colorBuf;

  char buf[256];
  serializeJson(doc, buf, sizeof(buf));
  mqttClient.publish(TOPIC_RPT_STATE, buf, true);
}

void mqttPublishSensor(int pirVal, int ldrVal, float temp, float hum,
                       bool hasPir, bool hasLdr, bool hasDht) {
  StaticJsonDocument<256> doc;
  buildEnvelope(doc, "rpt");
  JsonObject sensors = doc["data"].createNestedObject("sensors");
  if (hasPir) sensors["pir"]  = pirVal;
  if (hasLdr) sensors["ldr"]  = ldrVal;
  if (hasDht) { sensors["temp"] = temp; sensors["hum"] = hum; }

  char buf[256];
  serializeJson(doc, buf, sizeof(buf));
  mqttClient.publish(TOPIC_RPT_SENSOR, buf, false);
}
