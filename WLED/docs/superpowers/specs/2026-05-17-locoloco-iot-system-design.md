# LOCOLOCO IoT System Design Spec
**Date:** 2026-05-17  
**Status:** Approved  
**Scope:** Firmware V1.0.3 + Server Backend + Flutter App

---

## 1. Tổng quan hệ thống

Hệ thống điều khiển đèn LED RGB thông qua MQTT cho ~500 thiết bị ESP8266 NodeMCU v2. Gồm 3 subsystem độc lập được xây dựng theo thứ tự:

| Phase | Subsystem | Công nghệ |
|-------|-----------|-----------|
| 1 | Firmware ESP8266 | C++, FastLED, PubSubClient, ArduinoJson |
| 2 | Server Backend | Node.js, Express, PostgreSQL, Mosquitto, node-cron |
| 3 | Flutter App | Flutter, Riverpod, mqtt_client |

---

## 2. Kiến trúc tổng thể

```
┌─────────────────────────────────────────────────────┐
│                     VPS (Cloud)                     │
│                                                     │
│  ┌──────────────────┐      ┌───────────────────┐   │
│  │  Node.js Server  │      │ Mosquitto Broker  │   │
│  │  :3000 (HTTPS)   │◄────►│  :8883 (MQTTS)   │   │
│  └──────┬───────────┘      └────────┬──────────┘   │
│         │                           │               │
│  ┌──────▼──────────┐               │               │
│  │   PostgreSQL    │               │               │
│  │   :5432         │               │               │
│  └─────────────────┘               │               │
└─────────────────────────────────────┬───────────────┘
                                      │ MQTTS :8883
               ┌──────────────────────┼──────────────┐
               │                      │              │
      ┌────────▼──────┐      ┌────────▼──────┐      │
      │  ESP8266 #1   │      │  ESP8266 #2   │ ...  │
      │  PIR+DHT+LED  │      │  LDR+LED      │      │
      └───────────────┘      └───────────────┘      │
                                                     │
              ┌──────────────────────────────────────┘
              │
     ┌────────▼─────────────────────────────┐
     │           Flutter App                │
     │  REST → Server  (auth, device mgmt,  │
     │                  schedules, rules)   │
     │  MQTT → Broker  (commands, state     │
     │                  real-time sync)     │
     └──────────────────────────────────────┘
```

### Phân chia trách nhiệm

| Kênh | Ai dùng | Mục đích |
|------|---------|---------|
| REST HTTPS → Server | App | Login, CRUD device/schedule/rule |
| MQTT → Broker (pub) | App | Gửi lệnh LED trực tiếp |
| MQTT → Broker (sub) | App | Nhận state real-time từ thiết bị |
| MQTT → Broker (pub) | Server | Scheduler publish lệnh đúng giờ |
| MQTT → Broker (sub) | Server | Nhận sensor data → evaluate rule |
| MQTT → Broker | ESP8266 | Nhận lệnh, publish state + sensor |

### VPS sizing cho 500 thiết bị

- **Specs:** 2 vCPU, 2 GB RAM (≈ $10–12/tháng DigitalOcean)
- **MQTT keep-alive:** 120s (giảm connection overhead)
- **PostgreSQL:** chỉ persist khi state thay đổi, không persist mọi update
- **Mosquitto:** `max_connections 600`, `max_queued_messages 1000`

---

## 3. Message Architecture

Lấy cảm hứng từ Tuya IoT platform: Data Points + message envelope chuẩn hóa + topic phân cấp theo version và loại.

### 3.1 Topic Structure

```
loco/v1/{device_id}/cmd/led      ← app/server → device: lệnh LED
loco/v1/{device_id}/cmd/config   ← server → device: cập nhật config
loco/v1/{device_id}/rpt/state    ← device → app/server: trạng thái LED (retained)
loco/v1/{device_id}/rpt/sensor   ← device → server: dữ liệu cảm biến
loco/v1/{device_id}/evt/status   ← device → server: online/offline (LWT)
loco/v1/{device_id}/evt/alert    ← device → server: cảnh báo (mở rộng sau)
```

**Quy tắc versioning:** khi có breaking change dùng `loco/v2/...`, thiết bị cũ vẫn chạy `v1` không bị ảnh hưởng.

### 3.2 Message Envelope

Mọi message đều dùng wrapper này:

```json
{
  "v": 1,
  "msgId": "a3f9c1b2",
  "ts": 1716000000000,
  "devId": "esp_001",
  "type": "cmd",
  "data": { }
}
```

| Field | Type | Mục đích |
|-------|------|---------|
| `v` | int | Protocol version |
| `msgId` | string | ID unique per message, dùng để dedup và future ACK |
| `ts` | int64 | Unix timestamp ms, server detect stale message |
| `devId` | string | Device ID, redundant với topic nhưng giúp log/debug |
| `type` | string | `cmd` / `rpt` / `evt` |
| `data` | object | Payload |

### 3.3 Data Points (DPs)

Mỗi LED capability là 1 DP có ID số — thêm tính năng mới chỉ cần thêm DP, không đổi code cũ:

| DP | Tên | Type | Giá trị |
|----|-----|------|--------|
| 1 | switch | boolean | true/false |
| 2 | brightness | int | 0–255 |
| 3 | effect | int | 0–N (xem bảng effects) |
| 4 | color | string | "R,G,B" |
| 5 | color_temp | int | 2700–6500K *(mở rộng sau)* |
| 6 | segment | object | điều khiển từng đoạn *(mở rộng sau)* |

### 3.4 Message Examples

**Lệnh LED** (`cmd/led`, QoS 1):
```json
{
  "v": 1, "msgId": "a3f9c1b2", "ts": 1716000000000,
  "devId": "esp_001", "type": "cmd",
  "data": {
    "dps": { "1": true, "2": 180, "3": 2, "4": "255,100,0" }
  }
}
```

**State report** (`rpt/state`, retained, QoS 0):
```json
{
  "v": 1, "msgId": "b7d2e4f1", "ts": 1716000000010,
  "devId": "esp_001", "type": "rpt",
  "data": {
    "dps": { "1": true, "2": 180, "3": 2, "4": "255,100,0" }
  }
}
```

**Sensor report** (`rpt/sensor`, QoS 0):
```json
{
  "v": 1, "msgId": "c1a8b3d5", "ts": 1716000000020,
  "devId": "esp_001", "type": "rpt",
  "data": {
    "sensors": { "pir": 1, "ldr": 312, "temp": 28.5, "hum": 65.2 }
  }
}
```

**Online/Offline LWT** (`evt/status`):
```json
{
  "v": 1, "msgId": "d4f1c2a0", "ts": 1716000000030,
  "devId": "esp_001", "type": "evt",
  "data": { "online": false }
}
```

### 3.5 Subscribe Patterns

```
Server:  loco/v1/+/rpt/#    ← tất cả state + sensor mọi thiết bị
         loco/v1/+/evt/#    ← tất cả sự kiện

App:     loco/v1/{id}/rpt/state    ← state thiết bị đang xem
         loco/v1/{id}/evt/status   ← online/offline
```

---

## 4. Firmware ESP8266 (Phase 1)

Xây dựng trên nền V1.0.2, bump lên V1.0.3 với sensor support và message format mới.

### 4.1 Cấu trúc file

```
SW/LOCO_FIRME_V1.0.3/src/
  config.h           — WiFi, MQTT broker, pins, sensor flags, device_id
  main.cpp           — setup() / loop()
  led_control.h/cpp  — FastLED, 5 effects, apply DPs
  network.h/cpp      — WiFi connect/reconnect
  mqtt_handler.h/cpp — subscribe, parse envelope, publish state
  sensor.h/cpp       — PIR/LDR/DHT đọc, publish sensor report
```

### 4.2 config.h

```cpp
#define DEVICE_ID        "esp_001"
#define WIFI_SSID        "Loco"
#define WIFI_PASS        "matkhau1"
#define MQTT_HOST        "your-vps.com"
#define MQTT_PORT        8883
#define MQTT_USER        "esp_001"
#define MQTT_PASS        "device_password"
#define LED_PIN          2       // GPIO2/D4
#define LED_COUNT        30
#define LED_BRIGHTNESS   200

// Bật/tắt sensor theo phần cứng từng thiết bị
#define ENABLE_PIR       true    // GPIO5/D1
#define ENABLE_LDR       false   // A0
#define ENABLE_DHT       false   // GPIO4/D2

#define PIR_INTERVAL_MS  500
#define LDR_INTERVAL_MS  5000
#define DHT_INTERVAL_MS  30000
```

### 4.3 Effects

| Index | Tên |
|-------|-----|
| 0 | OFF |
| 1 | SOLID |
| 2 | BLINK |
| 3 | FADE |
| 4 | CHASE |

### 4.4 MQTT logic trên thiết bị

- Subscribe: `loco/v1/{device_id}/cmd/led`
- Publish state (retained, QoS 0): `loco/v1/{device_id}/rpt/state` — sau mỗi lần thay đổi LED
- Publish sensor (QoS 0): `loco/v1/{device_id}/rpt/sensor` — theo interval từng loại
- LWT: `loco/v1/{device_id}/evt/status` với `{"online": false}`
- Khi connect: publish `{"online": true}` lên `evt/status`

---

## 5. Server Backend (Phase 2)

### 5.1 Cấu trúc file

```
server/
  src/
    index.js
    routes/
      auth.js          — POST /api/auth/login
      devices.js       — CRUD /api/devices
      schedules.js     — CRUD /api/schedules
      rules.js         — CRUD /api/rules
    services/
      mqtt.service.js      — MQTT client, subscribe/publish, route messages
      scheduler.service.js — node-cron, load/reload jobs từ DB
      rule.service.js      — evaluate sensor rule → publish LED command
    db/
      schema.sql
      queries.js
    middleware/
      auth.js          — JWT verify middleware
```

### 5.2 PostgreSQL Schema

```sql
CREATE TABLE users (
  id            SERIAL PRIMARY KEY,
  username      VARCHAR(50) UNIQUE NOT NULL,
  password_hash VARCHAR(255) NOT NULL
);

CREATE TABLE devices (
  id         VARCHAR(32) PRIMARY KEY,    -- "esp_001"
  name       VARCHAR(100) NOT NULL,
  location   VARCHAR(100),
  online     BOOLEAN DEFAULT false,
  last_state JSONB,                       -- DPs snapshot
  created_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE TABLE schedules (
  id         SERIAL PRIMARY KEY,
  device_id  VARCHAR(32) REFERENCES devices(id) ON DELETE CASCADE,
  cron_expr  VARCHAR(50) NOT NULL,        -- "0 8 * * 1-5"
  command    JSONB NOT NULL,              -- DPs object
  enabled    BOOLEAN DEFAULT true,
  created_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE TABLE sensor_rules (
  id             SERIAL PRIMARY KEY,
  device_id      VARCHAR(32) REFERENCES devices(id) ON DELETE CASCADE,
  sensor_type    VARCHAR(10) NOT NULL,    -- "PIR", "LDR", "DHT_TEMP", "DHT_HUM"
  condition_op   VARCHAR(5) NOT NULL,     -- "==", ">", "<", ">=", "<="
  condition_val  FLOAT NOT NULL,
  command        JSONB NOT NULL,          -- DPs object
  enabled        BOOLEAN DEFAULT true,
  created_at     TIMESTAMPTZ DEFAULT NOW()
);
```

### 5.3 REST API

```
POST   /api/auth/login                   — {username, password} → {token, mqtt}
GET    /api/devices                      — list tất cả thiết bị
POST   /api/devices                      — đăng ký thiết bị mới
DELETE /api/devices/:id

GET    /api/schedules?device_id=:id
POST   /api/schedules
PUT    /api/schedules/:id
DELETE /api/schedules/:id

GET    /api/rules?device_id=:id
POST   /api/rules
PUT    /api/rules/:id
DELETE /api/rules/:id
```

**Login response:**
```json
{
  "token": "jwt...",
  "mqtt": {
    "host": "your-vps.com",
    "port": 8883,
    "username": "app_user",
    "password": "app_password"
  }
}
```

### 5.4 Sensor Rule Flow (Luồng A)

```
ESP8266 publish rpt/sensor
  → Mosquitto
  → Server mqtt.service.js (subscribe loco/v1/+/rpt/sensor)
  → rule.service.js: load rules cho device_id từ DB
  → evaluate: sensor_type + condition_op + condition_val
  → nếu match: build command envelope với DPs
  → publish loco/v1/{device_id}/cmd/led
  → ESP8266 nhận, thực thi LED
```

### 5.5 Scheduler Flow

```
Khởi động server → load tất cả schedules enabled từ DB
  → tạo node-cron job cho mỗi schedule
  → đúng giờ: publish loco/v1/{device_id}/cmd/led với command DPs

Khi app thêm/sửa/xóa schedule qua REST
  → update DB
  → reload cron jobs (cancel cũ, tạo mới)
```

---

## 6. Flutter App (Phase 3)

### 6.1 Màn hình

```
LoginScreen
  └── DeviceListScreen
        ├── DeviceDetailScreen
        │     ├── LedControlPanel   — on/off, effect picker, color picker, brightness
        │     ├── SensorPanel       — realtime PIR/LDR/DHT từ MQTT
        │     ├── ScheduleListScreen — CRUD lịch hẹn
        │     └── RuleListScreen    — CRUD sensor rules
        └── AddDeviceScreen
```

### 6.2 Kiến trúc App

- **State management:** Riverpod
- **HTTP client:** Dio (REST → Server)
- **MQTT client:** `mqtt_client` package
- **Credentials:** nhận từ server sau login, không hardcode

### 6.3 MQTT trong App

App subscribe sau khi mở DeviceDetailScreen:
```
loco/v1/{device_id}/rpt/state   → cập nhật LED UI real-time
loco/v1/{device_id}/evt/status  → hiển thị online/offline badge
```

App publish khi user điều khiển LED:
```
loco/v1/{device_id}/cmd/led    → gửi trực tiếp, không qua server
```

---

## 7. Bảo mật

| Layer | Cơ chế |
|-------|--------|
| App ↔ Server | HTTPS + JWT (HS256, expire 7 ngày) |
| App/Server ↔ Mosquitto | MQTTS (TLS port 8883) + username/password |
| Mỗi ESP8266 | MQTT username/password riêng biệt (= device_id + generated password) |
| Mosquitto ACL | Mỗi device chỉ pub/sub topic của chính nó |

---

## 8. Mở rộng trong tương lai

| Tính năng | Cách thêm |
|-----------|-----------|
| OTA firmware | Topic `cmd/ota`, `rpt/ota` — không đổi code cũ |
| Color temperature | Thêm DP 5 |
| Điều khiển từng đoạn LED | Thêm DP 6 |
| Sensor rule on ESP8266 | Topic `cmd/config` push rules xuống thiết bị |
| Multi-user | Thêm bảng `user_devices`, middleware phân quyền |
| Protocol v2 | Topic prefix `loco/v2/...`, thiết bị cũ vẫn dùng `v1` |
| Group control | Topic `loco/v1/group/{group_id}/cmd/led` |

---

## 9. Thứ tự xây dựng

1. **Phase 1 — Firmware V1.0.3:** cập nhật message format theo spec, thêm sensor support
2. **Phase 2 — Server:** Mosquitto setup, Node.js backend, PostgreSQL schema
3. **Phase 3 — Flutter App:** Login, device list, LED control, MQTT integration
4. **Phase 4 — Scheduling UI** (trong app)
5. **Phase 5 — Sensor Rules UI** (trong app)
