# Tài Liệu Dự Án WLED

## 1. Tổng Quan Dự Án
- **Tên**: WLED
- **Mục đích**: Điều khiển LED WS2812B qua WiFi/MQTT
- **Nền tảng**: ESP32-WROOM-IE
- **Framework**: Arduino/ESP-IDF

## 2. Cấu Trúc Dự Án

### 2.1 File Cốt Lõi
- `wled.h`: File header chính chứa các định nghĩa và khai báo
- `wled.cpp`: Triển khai chính của WLED
- `wled_main.cpp`: Điểm khởi đầu chương trình

### 2.2 Giao Thức Truyền Thông
- `mqtt.cpp`: Hỗ trợ MQTT
- `udp.cpp`: Xử lý UDP
- `network.cpp`: Quản lý kết nối mạng
- `ws.cpp`: Máy chủ WebSocket
- `remote.cpp`: Điều khiển từ xa

### 2.3 Điều Khiển LED
- `led.cpp`: Điều khiển LED cơ bản
- `FX.cpp` & `FX.h`: Hiệu ứng LED
- `FX_2Dfcn.cpp`: Hiệu ứng LED 2D
- `colors.cpp`: Quản lý màu sắc
- `palettes.h`: Bảng màu

### 2.4 Giao Diện Web
- `html_ui.h`: Giao diện người dùng web
- `html_settings.h`: Trang cài đặt
- `wled_server.cpp`: Máy chủ web

### 2.5 Cấu Hình
- `cfg.cpp`: Quản lý cấu hình
- `const.h`: Hằng số
- `my_config.h`: Cấu hình người dùng

## 3. Tính Năng Chính

### 3.1 Điều Khiển LED
- Hỗ trợ LED WS2812B
- Nhiều hiệu ứng có sẵn
- Điều chỉnh độ sáng và màu sắc
- Hỗ trợ ma trận 2D

### 3.2 Kết Nối
- WiFi (Chế độ AP & Client)
- MQTT
- UDP
- WebSocket
- Art-Net & E1.31

### 3.3 Giao Diện Người Dùng
- Giao diện web
- API JSON
- Điều khiển MQTT
- Điều khiển từ xa

### 3.4 Tính Năng Nâng Cao
- Preset & Playlist
- Sự kiện theo thời gian
- Phản ứng âm thanh
- Nhiều phân đoạn
- Hiệu ứng thời gian thực

## 4. Các Hàm Điều Khiển LED (`led.cpp`)

### 4.1 Hàm Quản Lý Phân Đoạn
```cpp
void setValuesFromMainSeg()
void setValuesFromFirstSelectedSeg()
void setValuesFromSegment(uint8_t s)
```
- Mục đích: Lấy giá trị màu và hiệu ứng từ phân đoạn chính hoặc đã chọn
- Giá trị: Màu chính (colPri), Màu phụ (colSec), hiệu ứng, tốc độ, cường độ, bảng màu

### 4.2 Hàm Áp Dụng Giá Trị
```cpp
void applyValuesToSelectedSegs()
```
- Áp dụng giá trị toàn cục cho các phân đoạn đã chọn
- Cập nhật: tốc độ, cường độ, bảng màu, chế độ hiệu ứng, màu sắc

### 4.3 Hàm Điều Khiển Độ Sáng
```cpp
void toggleOnOff()      // Bật/tắt LED
void scaledBri()        // Điều chỉnh độ sáng với hệ số
void applyBri()         // Áp dụng độ sáng tạm thời
void applyFinalBri()    // Áp dụng độ sáng cuối cùng
```

### 4.4 Hàm Quản Lý Trạng Thái
```cpp
void stateUpdated(byte callMode)     // Xử lý thay đổi trạng thái
void updateInterfaces(uint8_t callMode) // Cập nhật giao diện
void handleTransitions()             // Xử lý hiệu ứng chuyển tiếp
void colorUpdated(byte callMode)     // Cập nhật màu
void handleNightlight()              // Chế độ đèn ngủ
```

## 5. Hiệu Ứng LED (`FX.cpp`)

### 5.1 Hiệu Ứng Cơ Bản
```cpp
uint16_t mode_static()    // Đèn tĩnh
uint16_t mode_blink()     // Nhấp nháy
uint16_t mode_strobe()    // Đèn nháy nhanh
uint16_t mode_breath()    // Hiệu ứng thở
uint16_t mode_fade()      // Mờ dần
```

### 5.2 Hiệu Ứng Chuyển Động
```cpp
uint16_t mode_scan()           // Quét
uint16_t mode_dual_scan()      // Quét kép
uint16_t mode_running_lights() // Đèn chạy
uint16_t mode_comet()         // Sao băng
```

### 5.3 Hiệu Ứng Màu Sắc
```cpp
uint16_t mode_rainbow()     // Cầu vồng
uint16_t mode_colorwaves()  // Sóng màu
uint16_t mode_palette()     // Bảng màu
```

### 5.4 Hiệu Ứng Đặc Biệt
```cpp
uint16_t mode_fireworks()  // Pháo hoa
uint16_t mode_fire_2012()  // Lửa
uint16_t mode_lightning()  // Sấm chớp
uint16_t mode_ripple()     // Gợn sóng
```

### 5.5 Hiệu Ứng 2D
```cpp
uint16_t mode_2Dmatrix()  // Ma trận
uint16_t mode_2Dnoise()   // Nhiễu
uint16_t mode_2Dplasma()  // Plasma
```

### 5.6 Hiệu Ứng Hạt
```cpp
uint16_t mode_particles()     // Hệ thống hạt
uint16_t mode_particlefire()  // Lửa hạt
uint16_t mode_particlestorm() // Bão hạt
```

### 5.7 Hiệu Ứng Âm Thanh
```cpp
uint16_t mode_freqwave()   // Sóng tần số
uint16_t mode_gravfreq()   // Tần số trọng lực
uint16_t mode_waterfall()  // Thác nước
```

### 5.8 Cấu Trúc Hỗ Trợ
```cpp
struct Ripple {
    uint8_t state;    // Trạng thái
    uint8_t color;    // Màu sắc
    uint16_t pos;     // Vị trí
}

struct Ball {
    unsigned long lastBounceTime;  // Thời điểm nảy cuối
    float impactVelocity;         // Vận tốc va chạm
    float height;                 // Độ cao
}
```

### 5.9 Tham Số Hiệu Ứng
Mỗi hiệu ứng bao gồm:
- Tham số tốc độ
- Tham số cường độ
- Bảng màu
- Thời gian khung hình
- Trạng thái phân đoạn

### Lưu ý
Với esp32:
```cpp
#define MAX_LEDS 8192          // Số LED tối đa
#define MAX_LED_MEMORY 64000   // Bộ nhớ LED tối đa
#define WLED_MAX_BUSSES 24     // Số bus tối đa
#define OUTPUT_MAX_PINS 5      // Số chân tối đa cho mỗi output
```


# Tài Liệu WLED cho ESP32/ESP8266

## 1. Cấu Hình Chân GPIO cho ESP8266

### Chân GPIO Mặc Định
```cpp
// Chân mặc định cho WS2812B trên ESP8266
#define DEFAULT_LED_PIN 2    // GPIO2 (D4) trên Wemos D1 mini
```

### Bảng Ánh Xạ GPIO ESP8266
| NodeMCU/Wemos Pin | GPIO |
|-------------------|------|
| D0 | GPIO16 |
| D1 | GPIO5 (SCL) |
| D2 | GPIO4 (SDA) |
| D3 | GPIO0 |
| D4 | GPIO2 (LED_BUILTIN, LED mặc định) |
| D5 | GPIO14 (SCLK) |
| D6 | GPIO12 (MISO) |
| D7 | GPIO13 (MOSI) |
| D8 | GPIO15 |
| TX | GPIO1 |
| RX | GPIO3 |

## 2. Cấu Hình ESP32-WROOM-32

### Tổng Quan GPIO
```cpp
// Tổng số GPIO vật lý: 34 chân
// GPIO có thể sử dụng: 25 chân
// - GPIO0-GPIO19
// - GPIO21-GPIO23
// - GPIO25-GPIO27
// - GPIO32-GPIO33
```

### Phân Loại GPIO
1. **GPIO Input/Output (LED)**:
   - GPIO0-GPIO19
   - GPIO21-GPIO23 
   - GPIO25-GPIO27
   - GPIO32-GPIO33

2. **Input Only**:
   - GPIO34-GPIO39

3. **GPIO Đặc Biệt**:
   - GPIO0: Boot fail if pulled low
   - GPIO1: TX pin
   - GPIO3: RX pin
   - GPIO6-GPIO11: SPI flash
   - GPIO12: Boot fail if pulled high

## 3. Bus và Output Configuration

### Định Nghĩa Bus
- Bus là kênh điều khiển LED độc lập
- ESP32 hỗ trợ tối đa 24 bus
- Mỗi bus có thể điều khiển LED với cùng giao thức

### Ví Dụ Cấu Hình Bus

#### 1. Bus Đơn Giản (WS2812B - 1 chân)
```cpp
BusConfig config1 = {
    .type = TYPE_WS2812_RGB,
    .pins = {16, 255, 255, 255, 255}, // GPIO16
    .start = 0,
    .count = 60  // 60 LED
};
```

#### 2. Bus 2 Chân (APA102/SK9822)
```cpp
BusConfig config2 = {
    .type = TYPE_APA102,
    .pins = {23, 18, 255, 255, 255}, // Data + Clock
    .start = 0,
    .count = 60
};
```

#### 3. Bus Analog RGBW (4 chân)
```cpp
BusConfig config3 = {
    .type = TYPE_ANALOG_4CH,
    .pins = {16, 17, 18, 19, 255}, // R, G, B, W
    .start = 0,
    .count = 1
};
```

## 4. Điều Khiển Nhiều LED Strip

### Cách 1: Đấu Nối Tiếp (Recommended)
```cpp
BusConfig config = {
    .type = TYPE_WS2812_RGB,
    .pins = {16, 255, 255, 255, 255}, // GPIO16
    .start = 0,
    .count = 300,  // Tổng số LED của 5 dải
};
```

**Ưu điểm**:
- Chỉ cần 1 GPIO
- Dễ điều khiển
- Đồng bộ tốt

**Nhược điểm**:
- Không điều khiển riêng từng dải
- Tín hiệu có thể suy giảm

### Cách 2: Đấu Song Song
```cpp
BusConfig configs[] = {
    {TYPE_WS2812_RGB, {16}, 0, 60},  // Strip 1
    {TYPE_WS2812_RGB, {17}, 0, 60},  // Strip 2
    {TYPE_WS2812_RGB, {18}, 0, 60},  // Strip 3
    {TYPE_WS2812_RGB, {19}, 0, 60},  // Strip 4
    {TYPE_WS2812_RGB, {21}, 0, 60}   // Strip 5
};
```

**Ưu điểm**:
- Điều khiển độc lập
- Tín hiệu không suy giảm
- Hiệu ứng đa dạng

**Nhược điểm**:
- Tốn nhiều GPIO
- Phức tạp trong lập trình

## 5. Tính Toán Nguồn Điện
```cpp
// Công thức tính dòng điện:
LED_CURRENT = 60mA      // mỗi LED (RGB đầy đủ)
STRIP_LENGTH = 60       // LED mỗi dải
NUM_STRIPS = 5         // Số dải LED

Total_Current = LED_CURRENT * STRIP_LENGTH * NUM_STRIPS
// = 60mA * 60 * 5 = 18A
```

### Yêu Cầu Nguồn:
- Nguồn 5V/20A cho LED
- Tụ 1000μF gần mỗi dải LED
- GND chung ESP32 và LED

## 6. Sơ Đồ Đấu Nối

### Nối Tiếp:
```
┌─────────┐
│         │                  ┌──────┐    ┌──────┐    ┌──────┐
│  ESP32  ├──GPIO16─────DIN─┤Strip1├DOUT┤Strip2├DOUT┤Strip3├─...
│         │                  └──────┘    └──────┘    └──────┘
└─────────┘
```

### Song Song:
```
┌─────────┐
│         ├──GPIO16─────DIN─┤Strip1├
│         ├──GPIO17─────DIN─┤Strip2├
│  ESP32  ├──GPIO18─────DIN─┤Strip3├
│         ├──GPIO19─────DIN─┤Strip4├
│         ├──GPIO21─────DIN─┤Strip5├
└─────────┘
```

## 7. Giới Hạn Hệ Thống

### ESP32:
```cpp
#define WLED_MAX_BUSSES 24    // Số bus tối đa
#define MAX_LEDS 8192         // Số LED tối đa
#define MAX_LEDS_PER_BUS 2048 // SỐ LED tối đa trên mối bus
#define OUTPUT_MAX_PINS 5     // Số chân mỗi output
```

### ESP8266:
```cpp
#define MAX_LEDS 1664         // Số LED tối đa
#define MAX_LED_MEMORY 4000   // Bộ nhớ LED tối đa
```
Tôi sẽ giải thích cách thiết lập WLED cho cầu thang sử dụng ESP32. Đây là hướng dẫn chi tiết:

# Hướng Dẫn Cài Đặt WLED Cho Cầu Thang

## 1. Phân Tích Yêu Cầu

### Cấu Trúc Thông Thường của Cầu Thang
- Mỗi bậc thang cần 1 dải LED
- Thông thường cầu thang có 10-20 bậc
- Mỗi bậc có thể dùng 20-30 LED tùy chiều rộng

### Giải Pháp Đề Xuất
1. **Phương Pháp Đấu Nối**:
   - Đấu nối tiếp các dải LED (khuyến nghị)
   - Dùng 1 GPIO pin để điều khiển
   - Dễ lắp đặt và bảo trì

2. **Cảm Biến Chuyển Động**:
   - PIR sensor ở đầu và cuối cầu thang
   - Tự động bật/tắt khi có người

## 2. Cấu Hình Phần Cứng

### Linh Kiện Cần Thiết
```
1. ESP32 WROOM
2. Dải LED WS2812B
3. Nguồn 5V (công suất phù hợp)
4. 2 cảm biến PIR HC-SR501
5. Tụ điện 1000μF
6. Dây dẫn
```

### Tính Toán Nguồn Điện
```
Ví dụ cho cầu thang 15 bậc:
- Mỗi bậc: 30 LED
- Tổng LED: 15 x 30 = 450 LED
- Dòng điện: 450 x 60mA = 27A
=> Cần nguồn 5V/30A
```

## 3. Cấu Hình WLED

### Cài Đặt Ban Đầu
```cpp
// Trong file wled00/wled.h
#define LEDPIN 16  // Chân GPIO cho LED
#define BTNPIN -1  // Không dùng nút nhấn
#define IR_PIN -1  // Không dùng IR
#define RLYPIN -1  // Không dùng relay
```

### Cấu Hình LED
```cpp
// Trong platformio.ini
build_flags =
  -D WLED_MAX_LEDS=450
  -D WLED_MAX_SEGMENTS=15
  -D BTNPIN=-1
  -D RLYPIN=-1
  -D WLED_DISABLE_INFRARED
```

### Cấu Hình Cảm Biến
```cpp
// Thêm vào usermod.cpp
#define PIR_PIN_TOP 17     // Cảm biến đầu cầu thang
#define PIR_PIN_BOTTOM 18  // Cảm biến cuối cầu thang
```

## 4. Sơ Đồ Đấu Nối

```
┌─────────────────────┐
│       ESP32         │
├─────────────────────┤
│ GPIO16 ────────────┐│
│ GPIO17 ──── PIR1   ││
│ GPIO18 ──── PIR2   ││
│ GND ───────────────┘│
└─────────────────────┘
         │
    ┌────┘
    │    
┌───┴───┐    ┌───────┐    ┌───────┐
│Strip 1├────┤Strip 2├────┤Strip 3├──── ...
└───────┘    └───────┘    └───────┘
```

## 5. Hiệu Ứng Đề Xuất

### Hiệu Ứng Lên Cầu Thang
```json
{
  "name": "Lên Cầu Thang",
  "segments": [
    {
      "start": 0,
      "stop": 450,
      "effect": "Scan",
      "speed": 2000,
      "intensity": 128,
      "palette": "Default"
    }
  ]
}
```

### Hiệu Ứng Xuống Cầu Thang
```json
{
  "name": "Xuống Cầu Thang",
  "segments": [
    {
      "start": 0,
      "stop": 450,
      "effect": "Scan",
      "speed": 2000,
      "intensity": 128,
      "palette": "Default",
      "reverse": true
    }
  ]
}
```

## 6. Lắp Đặt Thực Tế

1. **Chuẩn Bị Bề Mặt**:
   - Làm sạch bề mặt cầu thang
   - Đánh dấu vị trí đặt LED
   - Chuẩn bị rãnh đi dây

2. **Lắp Đặt LED**:
   - Dán LED theo từng bậc
   - Nối tiếp các dải LED
   - Đảm bảo chiều mũi tên trên LED strip

3. **Lắp Đặt Cảm Biến**:
   - Đặt PIR1 ở đầu cầu thang
   - Đặt PIR2 ở cuối cầu thang
   - Điều chỉnh độ nhạy cảm biến

4. **Nguồn Điện**:
   - Đặt nguồn ở vị trí thông thoáng
   - Thêm tụ điện gần điểm cấp nguồn
   - Đảm bảo dây nguồn đủ tiết diện

## 7. Cài Đặt Phần Mềm

1. Nạp firmware WLED
2. Kết nối WiFi
3. Cấu hình LED:
   - Số lượng LED
   - Loại LED: WS2812B
   - Thứ tự màu: GRB

4. Tạo preset cho:
   - Lên cầu thang
   - Xuống cầu thang
   - Chế độ ban đêm (độ sáng thấp)

Bạn cần thêm thông tin chi tiết về phần nào không?
