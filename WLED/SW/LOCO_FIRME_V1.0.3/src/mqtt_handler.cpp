// SW/LOCO_FIRME_V1.0.3/src/mqtt_handler.cpp
#include "mqtt_handler.h"
#include "network.h"
#include "config.h"
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

static WiFiClientSecure wifiClient;
static PubSubClient     mqttClient(wifiClient);
static CommandCallback  cmdCallback;
static char             msgIdBuf[9];

// Option A: runtime device ID and topics built from MAC
static char devId[20];
static char topicCmdLed[64];
static char topicRptState[64];
static char topicRptSensor[64];
static char topicEvtStatus[64];
static const char* topicProvision = "loco/v1/provision";

static void buildTopics() {
  String id = getDeviceId();
  strlcpy(devId, id.c_str(), sizeof(devId));
  snprintf(topicCmdLed,    sizeof(topicCmdLed),    "loco/v1/%s/cmd/led",    devId);
  snprintf(topicRptState,  sizeof(topicRptState),  "loco/v1/%s/rpt/state",  devId);
  snprintf(topicRptSensor, sizeof(topicRptSensor), "loco/v1/%s/rpt/sensor", devId);
  snprintf(topicEvtStatus, sizeof(topicEvtStatus), "loco/v1/%s/evt/status", devId);
}

static void genMsgId() {
  uint32_t r = ESP.getCycleCount() ^ millis();
  snprintf(msgIdBuf, sizeof(msgIdBuf), "%08x", r);
}

static void buildEnvelope(JsonDocument& doc, const char* type) {
  genMsgId();
  doc["v"]     = PROTO_VERSION;
  doc["msgId"] = msgIdBuf;
  doc["ts"]    = (long long)millis();
  doc["devId"] = devId;
  doc["type"]  = type;
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

static void publishProvision() {
  StaticJsonDocument<200> doc;
  doc["devId"] = devId;
  // Default name can be changed by user in app later
  char nameBuf[32];
  snprintf(nameBuf, sizeof(nameBuf), "ESP-%s", devId + 4); // strip "esp_" prefix
  doc["name"] = nameBuf;
  doc["mac"]  = getMacStr().c_str();
  char buf[200];
  serializeJson(doc, buf, sizeof(buf));
  mqttClient.publish(topicProvision, buf, false);
  Serial.print("Provision published: ");
  Serial.println(devId);
}

static void mqttConnect() {
  char lwtPayload[192];
  StaticJsonDocument<192> lwtDoc;
  buildEnvelope(lwtDoc, "evt");
  lwtDoc["data"]["online"] = false;
  serializeJson(lwtDoc, lwtPayload, sizeof(lwtPayload));

  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(onMqttMessage);
  mqttClient.setKeepAlive(MQTT_KEEPALIVE);

  // Client ID = devId (unique per device), username = shared "device" user
  if (mqttClient.connect(devId, MQTT_USER, MQTT_PASS,
                         topicEvtStatus, 0, true, lwtPayload)) {
    mqttClient.subscribe(topicCmdLed, 1);

    // Option A: auto-register device with server on first connect
    publishProvision();

    // Publish online event
    StaticJsonDocument<192> onlineDoc;
    buildEnvelope(onlineDoc, "evt");
    onlineDoc["data"]["online"] = true;
    char onlineBuf[192];
    serializeJson(onlineDoc, onlineBuf, sizeof(onlineBuf));
    mqttClient.publish(topicEvtStatus, onlineBuf, true);
    Serial.println("MQTT connected");
  }
}

void mqttInit(CommandCallback onCmd) {
  cmdCallback = onCmd;
  buildTopics(); // build dynamic topics from MAC
  wifiClient.setInsecure();
  mqttConnect();
}

void mqttLoop() {
  static unsigned long lastReconnect = 0;
  if (!mqttClient.connected() && millis() - lastReconnect >= 5000) {
    lastReconnect = millis();
    mqttConnect();
  }
  mqttClient.loop();
}

bool mqttConnected() { return mqttClient.connected(); }

void mqttPublishState(bool on, uint8_t bri, uint8_t effect,
                      uint8_t r, uint8_t g, uint8_t b) {
  StaticJsonDocument<384> doc;
  buildEnvelope(doc, "rpt");
  JsonObject dps = doc["data"].createNestedObject("dps");
  dps["1"] = on;
  dps["2"] = bri;
  dps["3"] = effect;
  char colorBuf[16];
  snprintf(colorBuf, sizeof(colorBuf), "%d,%d,%d", r, g, b);
  dps["4"] = colorBuf;

  char buf[384];
  serializeJson(doc, buf, sizeof(buf));
  mqttClient.publish(topicRptState, buf, true);
}

void mqttPublishSensor(int pirVal, int ldrVal, float temp, float hum,
                       bool hasPir, bool hasLdr, bool hasDht) {
  StaticJsonDocument<384> doc;
  buildEnvelope(doc, "rpt");
  JsonObject sensors = doc["data"].createNestedObject("sensors");
  if (hasPir) sensors["pir"]  = pirVal;
  if (hasLdr) sensors["ldr"]  = ldrVal;
  if (hasDht) { sensors["temp"] = temp; sensors["hum"] = hum; }

  char buf[384];
  serializeJson(doc, buf, sizeof(buf));
  mqttClient.publish(topicRptSensor, buf, false);
}
