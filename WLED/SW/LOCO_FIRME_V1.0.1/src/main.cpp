#include <Arduino.h>
#include "led_control.h"

LEDControl ledControl;

void setup() {
    Serial.begin(115200);
    Serial.println("LED Control Demo");
    
    ledControl.begin();
    
    // Demo: Bật LED với màu đỏ
    ledControl.setPower(true);
    ledControl.setColor(255, 0, 0);  // Màu đỏ
    ledControl.setBrightness(128);   // 50% độ sáng
    ledControl.setEffect(EFFECT_SOLID);
}

void loop() {
    static uint32_t lastChange = 0;
    static uint8_t currentEffect = 0;
    
    // Cập nhật LED
    ledControl.update();
    
    // Mỗi 5 giây chuyển hiệu ứng
    if (millis() - lastChange >= 5000) {
        lastChange = millis();
        
        // Chuyển qua hiệu ứng tiếp theo
        currentEffect = (currentEffect + 1) % 5;
        ledControl.setEffect((EffectMode)currentEffect);
        
        // Đổi màu ngẫu nhiên
        ledControl.setColor(
            random(256),  // R
            random(256),  // G
            random(256)   // B
        );
        
        // In thông tin hiệu ứng hiện tại
        Serial.print("Effect: ");
        switch (currentEffect) {
            case EFFECT_OFF:    Serial.println("OFF"); break;
            case EFFECT_SOLID:  Serial.println("SOLID"); break;
            case EFFECT_BLINK:  Serial.println("BLINK"); break;
            case EFFECT_FADE:   Serial.println("FADE"); break;
            case EFFECT_CHASE:  Serial.println("CHASE"); break;
        }
    }
} 