// SW/LOCO_FIRME_V1.0.3/src/network.cpp
#include "network.h"
#include "storage.h"
#include "config.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>

static char wifiSsid[64];
static char wifiPass[64];

// ── Helpers ──────────────────────────────────────────────────────────────────

static String buildDevId() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char buf[20];
  snprintf(buf, sizeof(buf), "esp_%02x%02x%02x%02x", mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

String getDeviceId() { return buildDevId(); }

String getMacStr() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

// ── AP setup page (stored in flash, not RAM) ──────────────────────────────────

static const char SETUP_HTML[] PROGMEM = R"HTML(<!DOCTYPE html>
<html><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>LOCOLOCO Setup</title>
<style>
body{font-family:sans-serif;background:#f0f0f0;margin:0;padding:16px}
.box{background:#fff;border-radius:10px;padding:24px;max-width:400px;margin:auto;
     box-shadow:0 2px 12px rgba(0,0,0,.12)}
h2{color:#ff6b35;margin-top:0}
.id{font-size:12px;color:#888;margin-bottom:16px}
label{display:block;font-size:13px;color:#555;margin-top:14px}
input{width:100%;padding:10px;border:1px solid #ddd;border-radius:6px;
      box-sizing:border-box;font-size:15px;margin-top:4px}
button{width:100%;padding:13px;margin-top:22px;background:#ff6b35;color:#fff;
       border:none;border-radius:6px;font-size:16px;cursor:pointer}
button:active{background:#e55a28}
.note{font-size:12px;color:#999;margin-top:14px;text-align:center}
</style></head>
<body><div class="box">
<h2>&#x1F4A1; LOCOLOCO Setup</h2>
<div class="id">Device ID: %DEVID%</div>
<form method="POST" action="/save">
<label>WiFi SSID (t&#234;n m&#7841;ng)</label>
<input name="ssid" type="text" placeholder="Nh&#7853;p t&#234;n WiFi" required autocomplete="off">
<label>M&#7853;t kh&#7849;u WiFi</label>
<input name="pass" type="password" placeholder="&#272;&#7875; tr&#7889;ng n&#7871;u kh&#244;ng c&#243; m&#7853;t kh&#7849;u" autocomplete="off">
<button type="submit">&#x2705; L&#432;u &amp; k&#7871;t n&#7889;i</button>
</form>
<p class="note">ESP s&#7869; t&#7921; kh&#7903;i ?&#7897;ng v&#224; &#273;&#259;ng k&#253; v&#224;o h&#7879; th&#7889;ng</p>
</div></body></html>)HTML";

static const char SAVED_HTML[] PROGMEM = R"HTML(<!DOCTYPE html>
<html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Saved</title>
<style>body{font-family:sans-serif;text-align:center;padding:40px;color:#333}
h2{color:#4caf50}p{color:#666}</style></head>
<body><h2>&#x2705; &#272;&#227; l&#432;u!</h2>
<p>ESP &#273;ang kh&#7903;i ?&#7897;ng l&#7841;i v&#224; k&#7871;t n&#7889;i WiFi...</p>
<p>Thi&#7871;t b&#7883; s&#7869; t&#7921; xu&#7845;t hi&#7879;n trong app sau v&#224;i gi&#226;y.</p>
</body></html>)HTML";

// ── AP mode + captive portal ──────────────────────────────────────────────────

static ESP8266WebServer webServer(80);
static DNSServer        dnsServer;

static void handleRoot() {
  String page = FPSTR(SETUP_HTML);
  page.replace("%DEVID%", buildDevId());
  webServer.send(200, "text/html; charset=utf-8", page);
}

static void handleSave() {
  String ssid = webServer.arg("ssid");
  String pass = webServer.arg("pass");
  if (ssid.length() == 0) {
    webServer.send(400, "text/plain", "SSID required");
    return;
  }
  saveWifiConfig(ssid.c_str(), pass.c_str());
  webServer.send(200, "text/html; charset=utf-8", FPSTR(SAVED_HTML));
  delay(1500);
  ESP.restart();
}

static void startApMode() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char apSsid[32];
  snprintf(apSsid, sizeof(apSsid), "%s-%02X%02X", AP_SSID_PREFIX, mac[4], mac[5]);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSsid, strlen(AP_PASS) > 0 ? AP_PASS : nullptr);
  IPAddress apIP(192, 168, 4, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  // Captive portal: spoof all DNS → our IP so phone auto-opens setup page
  dnsServer.start(53, "*", apIP);

  // Handle all URLs → setup page (catches iOS/Android captive portal probes)
  webServer.on("/",     HTTP_GET,  handleRoot);
  webServer.on("/save", HTTP_POST, handleSave);
  webServer.onNotFound(handleRoot);
  webServer.begin();

  Serial.printf("AP mode: join WiFi '%s', open http://192.168.4.1\n", apSsid);

  // Block here — handleSave calls ESP.restart() after saving config
  while (true) {
    dnsServer.processNextRequest();
    webServer.handleClient();
    yield();
  }
}

// ── Public API ────────────────────────────────────────────────────────────────

void networkInit() {
  storageInit();

  // Check FLASH button (GPIO0, active LOW).
  // Press RESET on NodeMCU, then immediately hold FLASH for 3 seconds → AP mode.
  pinMode(SETUP_PIN, INPUT_PULLUP);
  if (digitalRead(SETUP_PIN) == LOW) {
    Serial.println("FLASH button held — hold 3s to enter setup mode...");
    unsigned long holdStart = millis();
    while (digitalRead(SETUP_PIN) == LOW) {
      if (millis() - holdStart >= SETUP_HOLD_MS) {
        Serial.println("Entering AP setup mode (WiFi config cleared)");
        clearWifiConfig();
        startApMode(); // does not return — restarts after save
      }
      delay(10);
    }
    // Released before 3s — continue normal boot
    Serial.println("Button released — continuing normal boot");
  }

  // Load saved WiFi credentials
  if (!loadWifiConfig(wifiSsid, wifiPass, sizeof(wifiSsid))) {
    Serial.println("No WiFi config — entering AP setup mode");
    startApMode(); // does not return
  }

  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSsid, wifiPass);
  Serial.printf("Connecting to '%s'", wifiSsid);
  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t < WIFI_TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi failed — entering AP setup mode");
    startApMode(); // does not return
  }

  Serial.printf("\nWiFi OK  IP: %s\n", WiFi.localIP().toString().c_str());
}

bool networkConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void networkLoop() {
  static unsigned long lastRetry = 0;
  if (WiFi.status() != WL_CONNECTED && millis() - lastRetry >= 5000) {
    lastRetry = millis();
    WiFi.reconnect();
  }
}
