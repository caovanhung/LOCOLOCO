// SW/LOCO_FIRME_V1.0.3/src/sensor.cpp
#include "sensor.h"
#include "config.h"
#include <DHT.h>

#if ENABLE_DHT
static DHT dht(DHT_PIN, DHT_TYPE);
#endif

static unsigned long lastPir = 0;
static unsigned long lastLdr = 0;
static unsigned long lastDht = 0;

void sensorInit() {
#if ENABLE_PIR
  pinMode(PIR_PIN, INPUT);
#endif
#if ENABLE_DHT
  dht.begin();
#endif
}

void sensorLoop(SensorPublishFn publishFn) {
  unsigned long now = millis();
  int pirVal = 0; int ldrVal = 0; float temp = 0; float hum = 0;
  bool readAny = false;

#if ENABLE_PIR
  if (now - lastPir >= PIR_INTERVAL_MS) {
    lastPir = now;
    pirVal = digitalRead(PIR_PIN);
    readAny = true;
  }
#endif

#if ENABLE_LDR
  if (now - lastLdr >= LDR_INTERVAL_MS) {
    lastLdr = now;
    ldrVal = analogRead(LDR_PIN);
    readAny = true;
  }
#endif

#if ENABLE_DHT
  if (now - lastDht >= DHT_INTERVAL_MS) {
    lastDht = now;
    temp = dht.readTemperature();
    hum  = dht.readHumidity();
    if (!isnan(temp) && !isnan(hum)) readAny = true;
  }
#endif

  if (readAny) {
    publishFn(pirVal, ldrVal, temp, hum, ENABLE_PIR, ENABLE_LDR, ENABLE_DHT);
  }
}
