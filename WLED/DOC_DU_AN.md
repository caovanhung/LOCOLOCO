# LOCOLOCO — Tài liệu dự án

**Cập nhật:** 2026-05-24

---

## Tổng quan

Hệ thống điều khiển đèn LED thông minh gồm 3 thành phần:
- **ESP8266 Firmware** — điều khiển LED, kết nối MQTT
- **Node.js Server** — API backend, quản lý user/device, xử lý MQTT
- **Flutter App** — ứng dụng mobile điều khiển thiết bị

---

## Kiến trúc hệ thống

```
[ESP8266]──MQTT──▶[Mosquitto Broker]◀──MQTT──[Node.js Server]◀──HTTPS──[Flutter App]
                        │                            │
                        └────────────────────────────┘
                              VPS: hung-test.codeaplha.biz
```

---

## 1. ESP8266 Firmware — V1.0.3

**Thư mục:** `SW/LOCO_FIRME_V1.0.3/`

### Cấu hình (`src/config.h`)

| Tham số | Giá trị |
|---------|---------|
| WiFi | Cấu hình qua AP setup (LittleFS `/wifi.json`) |
| AP SSID prefix | `LOCOLOCO` (mở, không pass) |
| Setup pin | GPIO0 / D3 / FLASH — giữ 3 giây sau reset |
| MQTT Host | `hung-test.codeaplha.biz` |
| MQTT Port | `1883` |
| MQTT User | `device` |
| MQTT Pass | `Loco@device2024` |
| LED Pin | GPIO2 / D4 |
| LED Count | 30 |
| LED Brightness | 200 |

### MQTT Topics (MAC-based device ID)

| Topic | Hướng | Nội dung |
|-------|-------|---------|
| `loco/v1/{devId}/cmd/led` | Subscribe | Lệnh điều khiển LED (JSON) |
| `loco/v1/{devId}/rpt/state` | Publish | Trạng thái LED (retained) |
| `loco/v1/{devId}/rpt/sensor` | Publish | Dữ liệu cảm biến |
| `loco/v1/{devId}/evt/status` | Publish | Online/offline (LWT) |
| `loco/v1/provision` | Publish | Tự đăng ký thiết bị khi kết nối lần đầu |

### Cấu trúc tin nhắn (Protocol v1)

```json
{
  "v": 1,
  "msgId": "a1b2c3d4",
  "ts": 1234567890,
  "devId": "esp_aabbccdd",
  "type": "cmd|rpt|evt",
  "data": {
    "dps": {
      "1": true,
      "2": 200,
      "3": 2,
      "4": "255,128,0"
    }
  }
}
```

**DPS (Data Points):**
- `1` — on/off (bool)
- `2` — brightness (0–255)
- `3` — effect (0–4)
- `4` — color "R,G,B"

### Cảm biến (bật/tắt trong config.h)

| Cảm biến | Flag | Pin |
|----------|------|-----|
| PIR | `ENABLE_PIR 1` | GPIO5 / D1 |
| LDR | `ENABLE_LDR 0` | A0 |
| DHT11 | `ENABLE_DHT 0` | GPIO4 / D2 |

---

## 2. Node.js Server

**Thư mục:** `server/`  
**Docker image:** `hungcao247/loco-server:latest`

### API Endpoints

#### Auth
| Method | Path | Mô tả |
|--------|------|-------|
| POST | `/api/auth/register` | Đăng ký tài khoản mới, gửi email xác minh |
| GET | `/api/auth/verify-email?token=xxx` | Xác minh email qua link |
| POST | `/api/auth/resend-verification` | Gửi lại email xác minh |
| POST | `/api/auth/login` | Đăng nhập, nhận JWT + MQTT credentials |

#### Devices
| Method | Path | Mô tả |
|--------|------|-------|
| GET | `/api/devices` | Danh sách thiết bị |
| POST | `/api/devices` | Thêm thiết bị thủ công |
| DELETE | `/api/devices/:id` | Xóa thiết bị |

#### Schedules
| Method | Path | Mô tả |
|--------|------|-------|
| GET | `/api/schedules?device_id=xxx` | Lịch của thiết bị |
| POST | `/api/schedules` | Tạo lịch mới |
| DELETE | `/api/schedules/:id` | Xóa lịch |

#### Rules
| Method | Path | Mô tả |
|--------|------|-------|
| GET | `/api/rules?device_id=xxx` | Rules của thiết bị |
| POST | `/api/rules` | Tạo rule mới |
| DELETE | `/api/rules/:id` | Xóa rule |

### Login Response

```json
{
  "token": "eyJ...",
  "mqtt": {
    "host": "hung-test.codeaplha.biz",
    "port": 1883,
    "username": "app_user",
    "password": "Loco@app2024"
  }
}
```

### Database Schema (PostgreSQL)

**users**
```sql
id, username, password_hash, email, email_verified, verification_token, token_expires_at
```

**devices**
```sql
id (MAC-based), name, location, online, last_state (JSONB), created_at
```

**schedules**
```sql
id, device_id, cron_expr, command (JSONB), enabled, created_at
```

**sensor_rules**
```sql
id, device_id, sensor_type, condition_op, condition_val, command (JSONB), enabled, created_at
```

### Biến môi trường (`.env`)

```env
PORT=3000
DATABASE_URL=postgres://loco:PASSWORD@postgres:5432/loco_db
JWT_SECRET=...
JWT_EXPIRES_IN=7d

MQTT_HOST=mosquitto
MQTT_PORT=1883
MQTT_USER=server
MQTT_PASS=Loco@server2024
MQTT_USE_TLS=false

APP_MQTT_HOST=hung-test.codeaplha.biz
APP_MQTT_PORT=1883
APP_MQTT_USER=app_user
APP_MQTT_PASS=Loco@app2024

GMAIL_USER=hung.cv.10@gmail.com
GMAIL_APP_PASS=<16-char App Password>
APP_BASE_URL=https://hung-test.codeaplha.biz
```

---

## 3. Flutter App

**Thư mục:** `app/`  
**Server URL:** `https://hung-test.codeaplha.biz`

### Màn hình

| Màn hình | File | Mô tả |
|----------|------|-------|
| LoginScreen | `screens/login_screen.dart` | Đăng nhập, link sang Register |
| RegisterScreen | `screens/register_screen.dart` | Đăng ký (username, email, password) |
| AwaitVerifyScreen | `screens/await_verify_screen.dart` | Chờ xác minh email, resend |
| DeviceListScreen | `screens/device_list_screen.dart` | Danh sách thiết bị |

### State Management

- **Riverpod** — `authProvider` (AuthState), `mqttServiceProvider`, `mqttConnectedProvider`
- `AuthState.isUnverified` — true khi login trả 403 (chưa xác minh email)

### Flow đăng ký

```
RegisterScreen → POST /register → AwaitVerifyScreen
                                        ↓
                              User click link email
                                        ↓
                              GET /verify-email?token=xxx
                                        ↓
                              LoginScreen → POST /login → DeviceListScreen
```

---

## 4. Mosquitto MQTT Broker

**Config:** `deploy/mosquitto/mosquitto.conf`  
**Port:** `1883` (plain, không TLS)

### Users

| User | Password | Dùng bởi |
|------|----------|---------|
| `device` | `Loco@device2024` | ESP8266/ESP32 firmware |
| `server` | `Loco@server2024` | Node.js server |
| `app_user` | `Loco@app2024` | Flutter app |

### Tạo passwd file trên VPS

```bash
docker run --rm -v /path/to/deploy/mosquitto:/mosquitto/config eclipse-mosquitto:2 \
  mosquitto_passwd -b /mosquitto/config/passwd device Loco@device2024
# lặp cho server và app_user
docker compose restart mosquitto
```

---

## 5. Deploy (VPS)

**Domain:** `hung-test.codeaplha.biz`  
**Thư mục:** `/var/opt/wled/WLED/deploy/`  
**Docker Compose:** `deploy/docker-compose.yml`

### Services

| Service | Image | Port |
|---------|-------|------|
| server | `hungcao247/loco-server:latest` | 3000 |
| postgres | `postgres:15-alpine` | internal |
| mosquitto | `eclipse-mosquitto:2` | 1883 |

### Deploy commands

```bash
cd /var/opt/wled/WLED/deploy
docker compose pull && docker compose up -d
```

### Migration DB (chạy 1 lần khi upgrade từ version cũ)

```bash
docker compose exec postgres psql -U loco -d loco_db -c "
  ALTER TABLE users ADD COLUMN IF NOT EXISTS email TEXT UNIQUE NOT NULL DEFAULT '';
  ALTER TABLE users ADD COLUMN IF NOT EXISTS email_verified BOOLEAN NOT NULL DEFAULT false;
  ALTER TABLE users ADD COLUMN IF NOT EXISTS verification_token TEXT;
  ALTER TABLE users ADD COLUMN IF NOT EXISTS token_expires_at TIMESTAMPTZ;
"
```

### CI/CD

- GitHub Actions tự động build Docker image khi push lên `main`
- Workflow: `.github/workflows/docker-build.yml`
- Image được push lên Docker Hub: `hungcao247/loco-server:latest`

---

## 6. Gmail SMTP (Email xác minh)

- Dùng **nodemailer** với Gmail SMTP (`smtp.gmail.com:465`, TLS)
- Yêu cầu **App Password** (không dùng password Gmail thường)
- Lấy App Password: `myaccount.google.com/apppasswords`
- Token xác minh hết hạn sau **24 giờ**
