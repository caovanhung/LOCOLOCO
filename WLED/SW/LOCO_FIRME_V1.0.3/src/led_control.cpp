// SW/LOCO_FIRME_V1.0.3/src/led_control.cpp
#include "led_control.h"
#include "config.h"
#include <FastLED.h>

static CRGB leds[LED_COUNT];
static LedState state;
static unsigned long lastBlink = 0;
static unsigned long lastFade  = 0;
static uint8_t fadeVal         = 0;
static bool fadeDir            = true;
static uint8_t chasePos        = 0;
static unsigned long lastChase = 0;

void ledInit() {
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, LED_COUNT);
  FastLED.setBrightness(LED_BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
}

static void effectOff()   { FastLED.clear(); FastLED.show(); }

static void effectSolid() {
  for (int i = 0; i < LED_COUNT; i++)
    leds[i] = CRGB(state.r, state.g, state.b);
  FastLED.setBrightness(state.bri);
  FastLED.show();
}

static void effectBlink() {
  if (millis() - lastBlink < 500) return;
  lastBlink = millis();
  static bool blinkOn = false;
  blinkOn = !blinkOn;
  if (blinkOn) effectSolid();
  else         { FastLED.clear(); FastLED.show(); }
}

static void effectFade() {
  if (millis() - lastFade < 10) return;
  lastFade = millis();
  if (fadeDir) {
    if (fadeVal < 255) fadeVal++;
    else fadeDir = false;
  } else {
    if (fadeVal > 0) fadeVal--;
    else fadeDir = true;
  }
  for (int i = 0; i < LED_COUNT; i++)
    leds[i] = CRGB(state.r, state.g, state.b);
  FastLED.setBrightness(fadeVal);
  FastLED.show();
}

static void effectChase() {
  if (millis() - lastChase < 80) return;
  lastChase = millis();
  FastLED.clear();
  leds[chasePos % LED_COUNT] = CRGB(state.r, state.g, state.b);
  chasePos = (chasePos + 1) % LED_COUNT;
  FastLED.setBrightness(state.bri);
  FastLED.show();
}

void ledLoop() {
  if (!state.on) { effectOff(); return; }
  switch (state.effect) {
    case 1: effectSolid(); break;
    case 2: effectBlink(); break;
    case 3: effectFade();  break;
    case 4: effectChase(); break;
    default: effectOff();  break;
  }
}

bool applyDPs(const char* dp1, int dp2, int dp3, const char* dp4) {
  LedState prev = state;
  if (dp1 != nullptr) state.on = (strcmp(dp1, "true") == 0);
  if (dp2 >= 0)       state.bri = (uint8_t)constrain(dp2, 0, 255);
  if (dp3 >= 0)       state.effect = (uint8_t)constrain(dp3, 0, 4);
  if (dp4 != nullptr) {
    char buf[16];
    strncpy(buf, dp4, 15); buf[15] = '\0';
    char* tok = strtok(buf, ",");
    if (tok) state.r = atoi(tok);
    tok = strtok(nullptr, ",");
    if (tok) state.g = atoi(tok);
    tok = strtok(nullptr, ",");
    if (tok) state.b = atoi(tok);
  }
  return (memcmp(&prev, &state, sizeof(LedState)) != 0);
}

LedState getLedState() { return state; }
