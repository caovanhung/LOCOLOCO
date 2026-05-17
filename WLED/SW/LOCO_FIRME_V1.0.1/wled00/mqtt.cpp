#include "wled.h"

/*
 * MQTT communication protocol for home automation
 */

#ifndef WLED_DISABLE_MQTT
#define MQTT_KEEP_ALIVE_TIME 60    // contact the MQTT broker every 60 seconds

#if MQTT_MAX_TOPIC_LEN > 32
#warning "MQTT topics length > 32 is not recommended for compatibility with usermods!"
#endif

#define CALL_MODE_MQTT 4  // Call mode for MQTT updates

static void parseMQTTBriPayload(char* payload)
{
  if      (strstr(payload, "ON") || strstr(payload, "on") || strstr(payload, "true")) {bri = briLast; stateUpdated(CALL_MODE_DIRECT_CHANGE);}
  else if (strstr(payload, "T" ) || strstr(payload, "t" )) {toggleOnOff(); stateUpdated(CALL_MODE_DIRECT_CHANGE);}
  else {
    uint8_t in = strtoul(payload, NULL, 10);
    if (in == 0 && bri > 0) briLast = bri;
    bri = in;
    stateUpdated(CALL_MODE_DIRECT_CHANGE);
  }
}


static void onMqttConnect(bool sessionPresent)
{
  //(re)subscribe to required topics
  char subuf[MQTT_MAX_TOPIC_LEN + 9];

  if (mqttDeviceTopic[0] != 0) {
    strlcpy(subuf, mqttDeviceTopic, MQTT_MAX_TOPIC_LEN + 1);
    mqtt->subscribe(subuf, 0);
    strcat_P(subuf, PSTR("/col"));
    mqtt->subscribe(subuf, 0);
    strlcpy(subuf, mqttDeviceTopic, MQTT_MAX_TOPIC_LEN + 1);
    strcat_P(subuf, PSTR("/api"));
    mqtt->subscribe(subuf, 0);
  }

  if (mqttGroupTopic[0] != 0) {
    strlcpy(subuf, mqttGroupTopic, MQTT_MAX_TOPIC_LEN + 1);
    mqtt->subscribe(subuf, 0);
    strcat_P(subuf, PSTR("/col"));
    mqtt->subscribe(subuf, 0);
    strlcpy(subuf, mqttGroupTopic, MQTT_MAX_TOPIC_LEN + 1);
    strcat_P(subuf, PSTR("/api"));
    mqtt->subscribe(subuf, 0);
  }

  // UsermodManager::onMqttConnect(sessionPresent);

  DEBUG_PRINTLN(F("MQTT ready"));

#ifndef USERMOD_SMARTNEST
  strlcpy(subuf, mqttDeviceTopic, MQTT_MAX_TOPIC_LEN + 1);
  strcat_P(subuf, PSTR("/status"));
  mqtt->publish(subuf, 0, true, "online"); // retain message for a LWT
#endif

  publishMqtt();
}


static void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  DEBUG_PRINTF_P(PSTR("MQTT message received: %s\n"), topic);
  static char *payloadStr = nullptr;
  static size_t payloadTotal = 0;
  
  if (index == 0) {
    // First chunk of message
    if (payloadStr != nullptr) {
      delete[] payloadStr;
      payloadStr = nullptr;
    }
    payloadTotal = total;
    if (total > 0) {
      payloadStr = new char[total + 1];
      if (payloadStr == nullptr) {
        DEBUG_PRINTLN(F("MQTT: Out of memory!"));
        return;
      }
    }
  }

  if (payloadStr != nullptr && len > 0) {
    memcpy(payloadStr + index, payload, len);
    if (index + len == total) {
      payloadStr[total] = '\0';
      
      // Check if this is a JSON message
      if (payloadStr[0] == '{') {
        DEBUG_PRINTLN(F("MQTT: Processing JSON payload"));
        StaticJsonDocument<1024> doc;  // Use StaticJsonDocument instead
        DeserializationError error = deserializeJson(doc, payloadStr);
        if (!error) {
          if (doc.containsKey("on")) {
            bool newState = doc["on"] | false;
            DEBUG_PRINTF_P(PSTR("MQTT: Setting power state to %d\n"), newState);
            if (newState != bri) {
              bri = newState ? briLast : 0;
              stateUpdated(CALL_MODE_DIRECT_CHANGE);
              DEBUG_PRINTF_P(PSTR("MQTT: Updated brightness to %d\n"), bri);
            }
          }
          // ... rest of JSON processing ...
        } else {
          DEBUG_PRINTF_P(PSTR("MQTT: JSON parsing failed: %s\n"), error.c_str());
        }
      }
      delete[] payloadStr;
      payloadStr = nullptr;
    }
  }

  // Debug log
  DEBUG_PRINTF_P(PSTR("MQTT msg: %s\n"), topic);
  DEBUG_PRINTF_P(PSTR("payload length: %d\n"), len);
  DEBUG_PRINTF_P(PSTR("payload index: %d\n"), index);
  DEBUG_PRINTF_P(PSTR("payload total: %d\n"), total);
  DEBUG_PRINTF_P(PSTR("Free heap: %d\n"), ESP.getFreeHeap());

  // Kiểm tra payload
  if (payload == nullptr) {
    DEBUG_PRINTLN(F("no payload -> leave"));
    return;
  }

  // Kiểm tra kích thước payload
  if (total > MQTT_MAX_PACKET_SIZE) {
    DEBUG_PRINTLN(F("Payload too large"));
    return;
  }

  // Xử lý payload từng phần
  if (index == 0) {
    // Reset buffer cho packet mới
    if (payloadStr) {
      free(payloadStr);
      payloadStr = nullptr;
    }
    payloadTotal = total;
    payloadStr = static_cast<char*>(malloc(total + 1));
    if (!payloadStr) {
      DEBUG_PRINTLN(F("Failed to allocate memory for payload"));
      return;
    }
    payloadStr[0] = '\0'; // Initialize as empty string
  }

  if (!payloadStr) {
    DEBUG_PRINTLN(F("No payload buffer"));
    return;
  }

  // Copy payload vào buffer
  if (index + len <= total) {
    memcpy(payloadStr + index, payload, len);
    payloadStr[index + len] = '\0';
  } else {
    DEBUG_PRINTLN(F("Invalid payload length"));
    free(payloadStr);
    payloadStr = nullptr;
    return;
  }

  // Chỉ xử lý khi nhận đủ packet
  if (index + len < total) {
    DEBUG_PRINTLN(F("MQTT partial packet received."));
    return;
  }

  // Validate JSON trước khi parse
  if (payloadStr[0] != '{' && payloadStr[0] != '[') {
    DEBUG_PRINTLN(F("Invalid JSON format"));
    free(payloadStr);
    payloadStr = nullptr;
    return;
  }

  DEBUG_PRINTF_P(PSTR("Complete payload: %s\n"), payloadStr);

  // Xử lý topic
  size_t topicPrefixLen = strlen(mqttDeviceTopic);
  if (strncmp(topic, mqttDeviceTopic, topicPrefixLen) == 0) {
    topic += topicPrefixLen;
  } else {
    topicPrefixLen = strlen(mqttGroupTopic);
    if (strncmp(topic, mqttGroupTopic, topicPrefixLen) == 0) {
      topic += topicPrefixLen;
    } else {
      // Non-Wled Topic
      UsermodManager::onMqttMessage(topic, payloadStr);
      free(payloadStr);
      payloadStr = nullptr;
      return;
    }
  }

  // Xử lý message theo topic
  if (strcmp_P(topic, PSTR("/col")) == 0) {
    DEBUG_PRINTF_P(PSTR("[MQTT] Set color: %s\n"), payloadStr);
    colorFromDecOrHexString(colPri, payloadStr);
    colorUpdated(CALL_MODE_DIRECT_CHANGE);
  } else if (strcmp_P(topic, PSTR("/api")) == 0) {
    DEBUG_PRINTF_P(PSTR("[MQTT] Set API: %s\n"), payloadStr);
    if (requestJSONBufferLock(15)) {
      if (payloadStr[0] == '{') { //JSON API
        DEBUG_PRINTF_P(PSTR("Processing JSON API: %s\n"), payloadStr);
        
        // Clear previous document
        pDoc->clear();
        
        // Parse JSON with error checking
        DeserializationError error = deserializeJson(*pDoc, payloadStr);
        if (error) {
          DEBUG_PRINTF_P(PSTR("JSON parse failed: %s\n"), error.c_str());
          releaseJSONBufferLock();
          return;
        }

        // Validate JSON structure
        JsonObject root = pDoc->as<JsonObject>();
        if (!root.isNull()) {
          DEBUG_PRINTLN(F("Valid JSON object received"));
          
          // Handle power state first
          if (root.containsKey("on")) {
            bool newState = root["on"] | false;
            DEBUG_PRINTF_P(PSTR("Setting power state to: %d\n"), newState);
            if (newState != bri) {
              bri = newState ? briLast : 0;
              stateUpdated(CALL_MODE_DIRECT_CHANGE);
            }
          }

          // Handle brightness
          if (root.containsKey("bri")) {
            uint8_t newBri = root["bri"] | bri;
            DEBUG_PRINTF_P(PSTR("Setting brightness to: %d\n"), newBri);
            if (newBri != bri) {
              bri = newBri;
              stateUpdated(CALL_MODE_DIRECT_CHANGE);
            }
          }

          // Handle color
          if (root.containsKey("col")) {
            JsonArray col = root["col"];
            if (col.size() >= 3) {
              uint8_t r = col[0][0] | 0;
              uint8_t g = col[0][1] | 0;
              uint8_t b = col[0][2] | 0;
              DEBUG_PRINTF_P(PSTR("Setting color to: [%d,%d,%d]\n"), r, g, b);
              col[0] = r;
              col[1] = g;
              col[2] = b;
              stateUpdated(CALL_MODE_DIRECT_CHANGE);
            }
          }

          // Apply all other state changes
          deserializeState(root);
          stateUpdated(CALL_MODE_DIRECT_CHANGE);
        } else {
          DEBUG_PRINTLN(F("Invalid JSON object"));
        }
      } else { //HTTP API
        String apireq = "win&";
        apireq += payloadStr;
        handleSet(nullptr, apireq);
      }
      releaseJSONBufferLock();
    }
  } else if (strlen(topic) != 0) {
    // non standard topic
    UsermodManager::onMqttMessage(topic, payloadStr);
  } else {
    // topmost topic
    parseMQTTBriPayload(payloadStr);
  }

  // Cleanup
  free(payloadStr);
  payloadStr = nullptr;
  payloadTotal = 0;
}

// Print adapter for flat buffers
namespace { 
class bufferPrint : public Print {
  char* _buf;
  size_t _size, _offset;
  public:

  bufferPrint(char* buf, size_t size) : _buf(buf), _size(size), _offset(0) {};

  size_t write(const uint8_t *buffer, size_t size) {
    size = std::min(size, _size - _offset);
    memcpy(_buf + _offset, buffer, size);
    _offset += size;
    return size;
  }

  size_t write(uint8_t c) {
    return this->write(&c, 1);
  }

  char* data() const { return _buf; }
  size_t size() const { return _offset; }
  size_t capacity() const { return _size; }
};
}; // anonymous namespace


void publishMqtt()
{
  if (!WLED_MQTT_CONNECTED) return;
  // DEBUG_PRINTLN(F("Publish MQTT"));

  #ifndef USERMOD_SMARTNEST
  char s[10];
  char subuf[MQTT_MAX_TOPIC_LEN + 16];

  sprintf_P(s, PSTR("%u"), bri);
  strlcpy(subuf, mqttDeviceTopic, MQTT_MAX_TOPIC_LEN + 1);
  strcat_P(subuf, PSTR("/g"));
  mqtt->publish(subuf, 0, retainMqttMsg, s);         // optionally retain message (#2263)

  sprintf_P(s, PSTR("#%06X"), (colPri[3] << 24) | (colPri[0] << 16) | (colPri[1] << 8) | (colPri[2]));
  strlcpy(subuf, mqttDeviceTopic, MQTT_MAX_TOPIC_LEN + 1);
  strcat_P(subuf, PSTR("/c"));
  mqtt->publish(subuf, 0, retainMqttMsg, s);         // optionally retain message (#2263)

  // TODO: use a DynamicBufferList.  Requires a list-read-capable MQTT client API.
  // DynamicBuffer buf(1024);
  // bufferPrint pbuf(buf.data(), buf.size());
  // XML_response(pbuf);
  // strlcpy(subuf, mqttDeviceTopic, MQTT_MAX_TOPIC_LEN + 1);
  // strcat_P(subuf, PSTR("/v"));
  // mqtt->publish(subuf, 0, retainMqttMsg, buf.data(), pbuf.size());   // optionally retain message (#2263)
  #endif
}


//HA autodiscovery was removed in favor of the native integration in HA v0.102.0

bool initMqtt()
{
  if (!mqttEnabled || mqttServer[0] == 0 || !WLED_CONNECTED) return false;

  if (mqtt == nullptr) {
    mqtt = new AsyncMqttClient();
    if (!mqtt) return false;
    mqtt->onMessage(onMqttMessage);
    mqtt->onConnect(onMqttConnect);
  }
  if (mqtt->connected()) return true;

  DEBUG_PRINTLN(F("Reconnecting MQTT"));
  IPAddress mqttIP;
  if (mqttIP.fromString(mqttServer)) //see if server is IP or domain
  {
    mqtt->setServer(mqttIP, mqttPort);
  } else {
    mqtt->setServer(mqttServer, mqttPort);
  }
  mqtt->setClientId(mqttClientID);
  if (mqttUser[0] && mqttPass[0]) mqtt->setCredentials(mqttUser, mqttPass);

  #ifndef USERMOD_SMARTNEST
  strlcpy(mqttStatusTopic, mqttDeviceTopic, MQTT_MAX_TOPIC_LEN + 1);
  strcat_P(mqttStatusTopic, PSTR("/status"));
  mqtt->setWill(mqttStatusTopic, 0, true, "offline"); // LWT message
  #endif
  mqtt->setKeepAlive(MQTT_KEEP_ALIVE_TIME);
  mqtt->connect();
  return true;
}
#endif
