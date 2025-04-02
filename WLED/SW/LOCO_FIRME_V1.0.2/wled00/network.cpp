#include "wled.h"
#include "fcn_declare.h"


//by https://github.com/tzapu/WiFiManager/blob/master/WiFiManager.cpp
int getSignalQuality(int rssi)
{
    int quality = 0;

    if (rssi <= -100)
    {
        quality = 0;
    }
    else if (rssi >= -50)
    {
        quality = 100;
    }
    else
    {
        quality = 2 * (rssi + 100);
    }
    return quality;
}

void fillMAC2Str(char *str, const uint8_t *mac) {
  sprintf_P(str, PSTR("%02x%02x%02x%02x%02x%02x"), MAC2STR(mac));
  byte nul = 0;
  for (int i = 0; i < 6; i++) nul |= *mac++;  // do we have 0
  if (!nul) str[0] = '\0';                    // empty string
}

void fillStr2MAC(uint8_t *mac, const char *str) {
  for (int i = 0; i < 6; i++) *mac++ = 0;     // clear
  if (!str) return;                           // null string
  uint64_t MAC = strtoull(str, nullptr, 16);
  for (int i = 0; i < 6; i++) { *--mac = MAC & 0xFF; MAC >>= 8; }
}


// performs asynchronous scan for available networks (which may take couple of seconds to finish)
// returns configured WiFi ID with the strongest signal (or default if no configured networks available)
int findWiFi(bool doScan) {
  if (multiWiFi.size() <= 1) {
    DEBUG_PRINTF_P(PSTR("WiFi: Defaulf SSID (%s) used.\n"), multiWiFi[0].clientSSID);
    return 0;
  }

  int status = WiFi.scanComplete(); // complete scan may take as much as several seconds (usually <6s with not very crowded air)

  if (doScan || status == WIFI_SCAN_FAILED) {
    DEBUG_PRINTF_P(PSTR("WiFi: Scan started. @ %lus\n"), millis()/1000);
    WiFi.scanNetworks(true);  // start scanning in asynchronous mode (will delete old scan)
  } else if (status >= 0) {   // status contains number of found networks (including duplicate SSIDs with different BSSID)
    DEBUG_PRINTF_P(PSTR("WiFi: Found %d SSIDs. @ %lus\n"), status, millis()/1000);
    int rssi = -9999;
    int selected = selectedWiFi;
    for (int o = 0; o < status; o++) {
      DEBUG_PRINTF_P(PSTR(" SSID: %s (BSSID: %s) RSSI: %ddB\n"), WiFi.SSID(o).c_str(), WiFi.BSSIDstr(o).c_str(), WiFi.RSSI(o));
      for (unsigned n = 0; n < multiWiFi.size(); n++)
        if (!strcmp(WiFi.SSID(o).c_str(), multiWiFi[n].clientSSID)) {
          bool foundBSSID = memcmp(multiWiFi[n].bssid, WiFi.BSSID(o), 6) == 0;
          // find the WiFi with the strongest signal (but keep priority of entry if signal difference is not big)
          if (foundBSSID || (n < selected && WiFi.RSSI(o) > rssi-10) || WiFi.RSSI(o) > rssi) {
            rssi = foundBSSID ? 0 : WiFi.RSSI(o); // RSSI is only ever negative
            selected = n;
          }
          break;
        }
    }
    DEBUG_PRINTF_P(PSTR("WiFi: Selected SSID: %s RSSI: %ddB\n"), multiWiFi[selected].clientSSID, rssi);
    return selected;
  }
  //DEBUG_PRINT(F("WiFi scan running."));
  return status; // scan is still running or there was an error
}

bool isWiFiConfigured() {
  return multiWiFi.size() > 1 || (strlen(multiWiFi[0].clientSSID) >= 1 && strcmp_P(multiWiFi[0].clientSSID, PSTR(DEFAULT_CLIENT_SSID)) != 0);
}

#if defined(ESP8266)
  #define ARDUINO_EVENT_WIFI_AP_STADISCONNECTED WIFI_EVENT_SOFTAPMODE_STADISCONNECTED
  #define ARDUINO_EVENT_WIFI_AP_STACONNECTED    WIFI_EVENT_SOFTAPMODE_STACONNECTED
  #define ARDUINO_EVENT_WIFI_STA_GOT_IP         WIFI_EVENT_STAMODE_GOT_IP
  #define ARDUINO_EVENT_WIFI_STA_CONNECTED      WIFI_EVENT_STAMODE_CONNECTED
  #define ARDUINO_EVENT_WIFI_STA_DISCONNECTED   WIFI_EVENT_STAMODE_DISCONNECTED
#elif defined(ARDUINO_ARCH_ESP32) && !defined(ESP_ARDUINO_VERSION_MAJOR) //ESP_IDF_VERSION_MAJOR==3
  // not strictly IDF v3 but Arduino core related
  #define ARDUINO_EVENT_WIFI_AP_STADISCONNECTED SYSTEM_EVENT_AP_STADISCONNECTED
  #define ARDUINO_EVENT_WIFI_AP_STACONNECTED    SYSTEM_EVENT_AP_STACONNECTED
  #define ARDUINO_EVENT_WIFI_STA_GOT_IP         SYSTEM_EVENT_STA_GOT_IP
  #define ARDUINO_EVENT_WIFI_STA_CONNECTED      SYSTEM_EVENT_STA_CONNECTED
  #define ARDUINO_EVENT_WIFI_STA_DISCONNECTED   SYSTEM_EVENT_STA_DISCONNECTED
  #define ARDUINO_EVENT_WIFI_AP_START           SYSTEM_EVENT_AP_START
  #define ARDUINO_EVENT_WIFI_AP_STOP            SYSTEM_EVENT_AP_STOP
  #define ARDUINO_EVENT_WIFI_SCAN_DONE          SYSTEM_EVENT_SCAN_DONE
  #define ARDUINO_EVENT_ETH_START               SYSTEM_EVENT_ETH_START
  #define ARDUINO_EVENT_ETH_CONNECTED           SYSTEM_EVENT_ETH_CONNECTED
  #define ARDUINO_EVENT_ETH_DISCONNECTED        SYSTEM_EVENT_ETH_DISCONNECTED
#endif

//handle Ethernet connection event
void WiFiEvent(WiFiEvent_t event)
{
  switch (event) {
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      // AP client disconnected
      if (--apClients == 0 && isWiFiConfigured()) forceReconnect = true; // no clients reconnect WiFi if awailable
      DEBUG_PRINTF_P(PSTR("WiFi-E: AP Client Disconnected (%d) @ %lus.\n"), (int)apClients, millis()/1000);
      break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      // AP client connected
      apClients++;
      DEBUG_PRINTF_P(PSTR("WiFi-E: AP Client Connected (%d) @ %lus.\n"), (int)apClients, millis()/1000);
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      DEBUG_PRINT(F("WiFi-E: IP address: ")); DEBUG_PRINTLN(Network.localIP());
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      // followed by IDLE and SCAN_DONE
      DEBUG_PRINTF_P(PSTR("WiFi-E: Connected! @ %lus\n"), millis()/1000);
      wasConnected = true;
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      if (wasConnected && interfacesInited) {
        DEBUG_PRINTF_P(PSTR("WiFi-E: Disconnected! @ %lus\n"), millis()/1000);
        if (interfacesInited && multiWiFi.size() > 1 && WiFi.scanComplete() >= 0) {
          findWiFi(true); // reinit WiFi scan
          forceReconnect = true;
        }
        interfacesInited = false;
      }
      break;
  #ifdef ARDUINO_ARCH_ESP32
    case ARDUINO_EVENT_WIFI_SCAN_DONE:
      // also triggered when connected to selected SSID
      DEBUG_PRINTLN(F("WiFi-E: SSID scan completed."));
      break;
    case ARDUINO_EVENT_WIFI_AP_START:
      DEBUG_PRINTLN(F("WiFi-E: AP Started"));
      break;
    case ARDUINO_EVENT_WIFI_AP_STOP:
      DEBUG_PRINTLN(F("WiFi-E: AP Stopped"));
      break;
    #if defined(WLED_USE_ETHERNET)
    case ARDUINO_EVENT_ETH_START:
      DEBUG_PRINTLN(F("ETH-E: Started"));
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      {
      DEBUG_PRINTLN(F("ETH-E: Connected"));
      if (!apActive) {
        WiFi.disconnect(true); // disable WiFi entirely
      }
      if (multiWiFi[0].staticIP != (uint32_t)0x00000000 && multiWiFi[0].staticGW != (uint32_t)0x00000000) {
        ETH.config(multiWiFi[0].staticIP, multiWiFi[0].staticGW, multiWiFi[0].staticSN, dnsAddress);
      } else {
        ETH.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
      }
      // convert the "serverDescription" into a valid DNS hostname (alphanumeric)
      char hostname[64];
      prepareHostname(hostname);
      ETH.setHostname(hostname);
      showWelcomePage = false;
      break;
      }
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      DEBUG_PRINTLN(F("ETH-E: Disconnected"));
      // This doesn't really affect ethernet per se,
      // as it's only configured once.  Rather, it
      // may be necessary to reconnect the WiFi when
      // ethernet disconnects, as a way to provide
      // alternative access to the device.
      if (interfacesInited && WiFi.scanComplete() >= 0) findWiFi(true); // reinit WiFi scan
      forceReconnect = true;
      break;
    #endif
  #endif
    default:
      DEBUG_PRINTF_P(PSTR("WiFi-E: Event %d\n"), (int)event);
      break;
  }
}

