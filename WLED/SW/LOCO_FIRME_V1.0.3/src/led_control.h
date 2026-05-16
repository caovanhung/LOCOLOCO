// SW/LOCO_FIRME_V1.0.3/src/led_control.h
#pragma once
#include <Arduino.h>

struct LedState {
  bool on         = false;
  uint8_t bri     = 200;   // DP2
  uint8_t effect  = 0;     // DP3: 0=OFF 1=SOLID 2=BLINK 3=FADE 4=CHASE
  uint8_t r       = 255;   // DP4 red
  uint8_t g       = 255;
  uint8_t b       = 255;
};

void ledInit();
void ledLoop();
// Áp dụng DPs từ JSON object, trả về true nếu state thay đổi
bool applyDPs(const char* dp1,  // "true"/"false"/nullptr
              int dp2,          // -1 = không đổi
              int dp3,          // -1 = không đổi
              const char* dp4); // "R,G,B" hoặc nullptr
LedState getLedState();
