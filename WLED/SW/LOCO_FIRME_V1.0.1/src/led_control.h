#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <FastLED.h>

// Cấu hình LED
#define LED_PIN     2    // Chân GPIO kết nối với LED
#define NUM_LEDS    30   // Số lượng LED trong chuỗi
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

// Các chế độ hiệu ứng
enum EffectMode {
    EFFECT_OFF = 0,
    EFFECT_SOLID,    // Màu đơn
    EFFECT_BLINK,    // Nhấp nháy
    EFFECT_FADE,     // Fade in/out
    EFFECT_CHASE     // Chạy đuổi
};

class LEDControl {
private:
    CRGB leds[NUM_LEDS];
    EffectMode currentMode;
    uint8_t brightness;
    CRGB currentColor;
    uint32_t lastUpdate;
    uint8_t effectStep;
    
    void updateEffect();

public:
    LEDControl();
    void begin();
    void update();
    
    // Điều khiển cơ bản
    void setPower(bool on);
    void setBrightness(uint8_t brightness);
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void setEffect(EffectMode mode);
    
    // Lấy trạng thái
    bool isOn();
    uint8_t getBrightness();
    CRGB getColor();
    EffectMode getEffect();
};

#endif 