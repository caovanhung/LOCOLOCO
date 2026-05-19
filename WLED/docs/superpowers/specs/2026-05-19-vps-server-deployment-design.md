# VPS Server Deployment Design

**Date:** 2026-05-19  
**Status:** Approved

## Overview

Deploy the existing Node.js server (REST API + MQTT client) to a VPS with a proper domain and TLS, replacing the earlier Jetson Nano + ngrok concept. All backend services run on the VPS via Docker Compose. The Flutter app connects from the internet (4G/WiFi).

## Architecture

```
Internet
  ├── :443  HTTPS  →  Nginx  →  server:3000   (REST API)
  ├── :80   HTTP   →  Nginx  →  redirect 443 + certbot webroot
  └── :8883 TLS    →  Nginx stream  →  mosquitto:1883  (MQTT)

VPS — Docker Compose
  ├── nginx       nginx:alpine          TLS termination + reverse proxy
  ├── server      build ./server        Node.js Express REST API
  ├── postgres    postgres:15-alpine    PostgreSQL database
  ├── mosquitto   eclipse-mosquitto:2   MQTT broker (plain, internal only)
  └── certbot     certbot/certbot       Let's Encrypt cert lifecycle
```

**Key decision:** Nginx handles TLS termination for both HTTPS (port 443) and MQTT TLS (port 8883) via the `stream` module. Mosquitto runs plain MQTT on port 1883 reachable only within the Docker network — no cert management needed in Mosquitto itself.

## Services

### nginx
- Handles all inbound TLS.
- HTTP block: serves certbot webroot challenge at `/.well-known/acme-challenge/`, redirects everything else to HTTPS.
- HTTPS block: proxies to `server:3000`, sets standard proxy headers.
- Stream block: TCP proxy on port 8883 (SSL termination) → `mosquitto:1883`.
- Cert path: `/etc/letsencrypt/live/<domain>/fullchain.pem` and `privkey.pem`, mounted read-only from the `certbot_conf` volume.

### server
- Built from `server/Dockerfile` (Node.js 20 LTS, non-root user).
- Reads config from environment variables (`.env` file).
- Connects to `postgres:5432` and `mosquitto:1883` over the internal Docker network — no TLS needed for these internal connections.
- `MQTT_USE_TLS=false` in the `.env` for the server's MQTT connection to Mosquitto.

### postgres
- postgres:15-alpine, data persisted to a named volume.
- Credentials set via `POSTGRES_USER`, `POSTGRES_PASSWORD`, `POSTGRES_DB` env vars.
- Schema applied via `server/src/db/schema.sql` on first run (init script or manual).

### mosquitto
- eclipse-mosquitto:2 with custom `mosquitto.conf`.
- Listens on port 1883 (plain) — internal only, not published to host.
- Password authentication enabled (`passwd` file mounted).
- Two MQTT users: `server` (subscribe/publish all topics) and `app_user` (subscribe/publish `wled/#`).
- Persistence enabled with data volume.

### certbot
- certbot/certbot image used to issue and renew Let's Encrypt certs.
- First-time issuance: `init-letsencrypt.sh` script runs `certbot certonly --webroot`.
- Renewal: `docker compose run certbot renew` can be scheduled via host cron (weekly).
- Shares `certbot_conf` and `certbot_www` volumes with nginx.

## File Layout

```
deploy/
├── docker-compose.yml
├── .env                        production env vars (not committed)
├── .env.example                template with placeholders
├── init-letsencrypt.sh         one-time cert issuance script
├── nginx/
│   └── nginx.conf
└── mosquitto/
    ├── mosquitto.conf
    └── passwd                  generated with mosquitto_passwd

server/
└── Dockerfile
```

## Environment Variables (deploy/.env)

```
DOMAIN=api.yourdomain.com

PORT=3000
DATABASE_URL=postgres://loco:<password>@postgres:5432/loco_db
JWT_SECRET=<64-char random string>
JWT_EXPIRES_IN=7d

# Internal: server → Mosquitto (Docker network, no TLS)
MQTT_HOST=mosquitto
MQTT_PORT=1883
MQTT_USER=server
MQTT_PASS=<server_mqtt_password>
MQTT_USE_TLS=false

# Public: returned to Flutter app in login response
APP_MQTT_HOST=api.yourdomain.com
APP_MQTT_PORT=8883
APP_MQTT_USER=app_user
APP_MQTT_PASS=<app_mqtt_password>

POSTGRES_USER=loco
POSTGRES_PASSWORD=<db_password>
POSTGRES_DB=loco_db
```

## MQTT Credentials Flow

`server/src/routes/auth.js` hiện trả về `MQTT_HOST`/`MQTT_PORT` (Docker internal: `mosquitto:1883`) cho Flutter app — sẽ không kết nối được từ ngoài. Cần sửa `auth.js` để dùng `APP_MQTT_HOST`/`APP_MQTT_PORT`:

```js
// auth.js — thay
host: process.env.MQTT_HOST,
port: Number(process.env.MQTT_PORT) || 8883,
// thành
host: process.env.APP_MQTT_HOST,
port: Number(process.env.APP_MQTT_PORT) || 8883,
```

Flow sau khi sửa:
1. Flutter app calls `POST /api/auth/login`.
2. Server responds with JWT token + MQTT credentials:
   ```json
   { "token": "...", "mqtt": { "host": "api.yourdomain.com", "port": 8883, "username": "app_user", "password": "..." } }
   ```
3. Flutter app connects to `api.yourdomain.com:8883` with TLS using `app_user` credentials.
4. `APP_MQTT_HOST`, `APP_MQTT_PORT`, `APP_MQTT_USER`, `APP_MQTT_PASS` đều read từ server env.

## App Change

`app/lib/providers/auth_provider.dart` line 4:
```dart
// before
const _serverUrl = 'https://your-vps.com:3000';
// after
const _serverUrl = 'https://api.yourdomain.com';
```

This is a placeholder — actual domain filled in after VPS is provisioned.

## Deployment Steps (high level)

1. Provision VPS (Ubuntu 22.04+), install Docker + Docker Compose.
2. Point domain A record → VPS public IP.
3. Clone repo, create `deploy/.env` from `.env.example`.
4. Generate Mosquitto passwd file.
5. Run `init-letsencrypt.sh` to issue Let's Encrypt cert.
6. `docker compose up -d` — all services start.
7. Apply DB schema: `docker compose exec server node -e "require('./src/db/queries').initSchema()"` or via psql.
8. Create initial user via API or direct DB insert.
9. Update `_serverUrl` in Flutter app, rebuild.

## Security Notes

- Port 1883 (plain MQTT) must NOT be published to the host — it is Docker-internal only.
- Port 5432 (PostgreSQL) must NOT be published to the host.
- `deploy/.env` must not be committed to git — add to `.gitignore`.
- JWT_SECRET must be at least 64 random characters.
- Use firewall (ufw) on VPS: allow only ports 22, 80, 443, 8883.

## Out of Scope

- CI/CD pipeline
- Database backups
- Log aggregation
- Multi-VPS / high availability
