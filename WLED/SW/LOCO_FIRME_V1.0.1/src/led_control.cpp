#include "led_control.h"

LEDControl::LEDControl() {
    currentMode = EFFECT_OFF;
    brightness = 255;
    currentColor = CRGB::Black;
    lastUpdate = 0;
    effectStep = 0;
}

void LEDControl::begin() {
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(brightness);
    FastLED.clear();
    FastLED.show();
}

void LEDControl::update() {
    uint32_t now = millis();
    
    // Cập nhật hiệu ứng mỗi 20ms
    if (now - lastUpdate >= 20) {
        lastUpdate = now;
        updateEffect();
        FastLED.show();
    }
}

void LEDControl::updateEffect() {
    switch (currentMode) {
        case EFFECT_OFF:
            FastLED.clear();
            break;
            
        case EFFECT_SOLID:
            fill_solid(leds, NUM_LEDS, currentColor);
            break;
            
        case EFFECT_BLINK:
            if (effectStep < 25) {  // 500ms on
                fill_solid(leds, NUM_LEDS, currentColor);
            } else {                // 500ms off
                FastLED.clear();
            }
            effectStep = (effectStep + 1) % 50;
            break;
            
        case EFFECT_FADE:
            {
                uint8_t brightness = (effectStep < 128) ? effectStep * 2 : (255 - effectStep) * 2;
                fill_solid(leds, NUM_LEDS, currentColor);
                FastLED.setBrightness(brightness);
                effectStep = (effectStep + 1) % 256;
            }
            break;
            
        case EFFECT_CHASE:
            {
                FastLED.clear();
                for (int i = 0; i < NUM_LEDS; i += 3) {
                    int pos = (i + effectStep) % NUM_LEDS;
                    leds[pos] = currentColor;
                }
                effectStep = (effectStep + 1) % NUM_LEDS;
            }
            break;
    }
}

void LEDControl::setPower(bool on) {
    if (!on) {
        currentMode = EFFECT_OFF;
        FastLED.clear();
        FastLED.show();
    } else if (currentMode == EFFECT_OFF) {
        currentMode = EFFECT_SOLID;
    }
}

void LEDControl::setBrightness(uint8_t newBrightness) {
    brightness = newBrightness;
    FastLED.setBrightness(brightness);
}

void LEDControl::setColor(uint8_t r, uint8_t g, uint8_t b) {
    currentColor = CRGB(r, g, b);
}

void LEDControl::setEffect(EffectMode mode) {
    currentMode = mode;
    effectStep = 0;
    if (mode == EFFECT_OFF) {
        FastLED.clear();
        FastLED.show();
    }
}

bool LEDControl::isOn() {
    return currentMode != EFFECT_OFF;
}

uint8_t LEDControl::getBrightness() {
    return brightness;
}

CRGB LEDControl::getColor() {
    return currentColor;
}

EffectMode LEDControl::getEffect() {
    return currentMode;
} 