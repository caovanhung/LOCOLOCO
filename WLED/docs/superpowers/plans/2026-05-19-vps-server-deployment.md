# VPS Server Deployment — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Deploy the existing Node.js server (REST API + MQTT client) to a VPS at `115.146.127.154` behind the domain `hung-test.codeaplha.biz`, with TLS termination via Nginx and a Docker Compose stack.

**Architecture:** Nginx handles all inbound TLS — HTTPS on port 443 proxied to Node.js, and MQTT TLS on port 8883 stream-proxied to Mosquitto's plain port 1883. PostgreSQL and Mosquitto are Docker-internal only. Let's Encrypt certs are obtained via certbot webroot challenge through Nginx's HTTP block.

**Tech Stack:** Docker Compose, Node.js 20, PostgreSQL 15-alpine, Eclipse Mosquitto 2, Nginx alpine, Certbot/certbot, Flutter (Dart).

---

## File Map

| Action | Path |
|--------|------|
| Create | `server/Dockerfile` |
| Create | `server/.dockerignore` |
| Modify | `server/src/routes/auth.js:28-29` |
| Modify | `server/tests/auth.test.js` |
| Create | `.gitignore` (repo root) |
| Create | `deploy/.env.example` |
| Create | `deploy/mosquitto/mosquitto.conf` |
| Create | `deploy/nginx/nginx.conf.template` |
| Create | `deploy/docker-compose.yml` |
| Create | `deploy/init-letsencrypt.sh` |
| Modify | `app/lib/providers/auth_provider.dart:4` |

---

## Task 1: Fix auth.js — return APP_MQTT_HOST to Flutter clients (TDD)

**Problem:** `auth.js:28` returns `process.env.MQTT_HOST` (`mosquitto` — Docker-internal hostname) in the login response. The Flutter app tries to connect to that host directly and fails. Fix uses two new env vars: `APP_MQTT_HOST` and `APP_MQTT_PORT` for the public-facing MQTT endpoint.

**Files:**
- Modify: `server/tests/auth.test.js`
- Modify: `server/src/routes/auth.js:28-29`

- [ ] **Step 1: Add a failing test to `server/tests/auth.test.js`**

  Open `server/tests/auth.test.js` and add this test inside the existing `describe` block, after the last `it(...)`:

  ```js
  it('returns APP_MQTT_HOST/PORT in mqtt credentials, not internal MQTT_HOST', async () => {
    const prev = { host: process.env.MQTT_HOST, port: process.env.MQTT_PORT };
    process.env.MQTT_HOST = 'internal-broker';
    process.env.APP_MQTT_HOST = 'hung-test.codeaplha.biz';
    process.env.APP_MQTT_PORT = '8883';

    const res = await request(app)
      .post('/api/auth/login')
      .send({ username: 'admin', password: 'admin123' });

    process.env.MQTT_HOST = prev.host;
    process.env.MQTT_PORT = prev.port;
    delete process.env.APP_MQTT_HOST;
    delete process.env.APP_MQTT_PORT;

    expect(res.status).toBe(200);
    expect(res.body.mqtt.host).toBe('hung-test.codeaplha.biz');
    expect(res.body.mqtt.port).toBe(8883);
  });
  ```

- [ ] **Step 2: Run the test to confirm it fails**

  ```
  cd server
  npm test -- --testPathPattern=auth --verbose
  ```

  Expected: FAIL — `expect(received).toBe(expected) — Expected: "hung-test.codeaplha.biz", Received: "internal-broker"`

- [ ] **Step 3: Fix `server/src/routes/auth.js:28-29`**

  In `auth.js` inside the `res.json({ token, mqtt: { ... } })` call, change:

  ```js
  // before (lines 28-29)
  host: process.env.MQTT_HOST,
  port: Number(process.env.MQTT_PORT) || 8883,
  ```

  to:

  ```js
  host: process.env.APP_MQTT_HOST,
  port: Number(process.env.APP_MQTT_PORT) || 8883,
  ```

- [ ] **Step 4: Run all tests to confirm they pass**

  ```
  npm test -- --verbose
  ```

  Expected: all tests PASS (the new test + existing tests).

- [ ] **Step 5: Commit**

  ```bash
  cd ..
  git add server/src/routes/auth.js server/tests/auth.test.js
  git commit -m "fix(server): return APP_MQTT_HOST/PORT to Flutter clients in login response"
  ```

---

## Task 2: server/Dockerfile + .dockerignore

**Files:**
- Create: `server/Dockerfile`
- Create: `server/.dockerignore`

- [ ] **Step 1: Create `server/Dockerfile`**

  ```dockerfile
  FROM node:20-alpine
  WORKDIR /app
  COPY package*.json ./
  RUN npm ci --omit=dev
  COPY src/ ./src/
  USER node
  EXPOSE 3000
  CMD ["node", "src/index.js"]
  ```

- [ ] **Step 2: Create `server/.dockerignore`**

  ```
  node_modules/
  .env
  *.log
  tests/
  coverage/
  ```

- [ ] **Step 3: Verify the image builds**

  ```
  cd server
  docker build -t loco-server:local .
  ```

  Expected: `Successfully built <id>` and `Successfully tagged loco-server:local`.

- [ ] **Step 4: Commit**

  ```bash
  cd ..
  git add server/Dockerfile server/.dockerignore
  git commit -m "feat(server): add Dockerfile for production Docker image"
  ```

---

## Task 3: Root .gitignore + deploy/.env.example

**Files:**
- Create: `.gitignore` (repo root)
- Create: `deploy/.env.example`

- [ ] **Step 1: Create `.gitignore` at the repo root**

  ```
  # Production secrets — never commit
  deploy/.env
  deploy/certbot/
  deploy/mosquitto/passwd
  ```

- [ ] **Step 2: Create `deploy/.env.example`**

  ```
  # VPS domain (A record must point to this VPS before running init-letsencrypt.sh)
  DOMAIN=hung-test.codeaplha.biz

  # Let's Encrypt — your email for expiry notifications
  CERTBOT_EMAIL=your@email.com

  # Node.js server
  PORT=3000
  JWT_SECRET=change-this-to-a-64-char-random-string
  JWT_EXPIRES_IN=7d

  # Database (used by server + postgres service)
  DATABASE_URL=postgres://loco:CHANGE_DB_PASS@postgres:5432/loco_db
  POSTGRES_USER=loco
  POSTGRES_PASSWORD=CHANGE_DB_PASS
  POSTGRES_DB=loco_db

  # Internal server → Mosquitto (Docker network, no TLS needed)
  MQTT_HOST=mosquitto
  MQTT_PORT=1883
  MQTT_USER=server
  MQTT_PASS=CHANGE_SERVER_MQTT_PASS
  MQTT_USE_TLS=false

  # Public MQTT endpoint returned to Flutter app after login
  APP_MQTT_HOST=hung-test.codeaplha.biz
  APP_MQTT_PORT=8883
  APP_MQTT_USER=app_user
  APP_MQTT_PASS=CHANGE_APP_MQTT_PASS
  ```

- [ ] **Step 3: Commit**

  ```bash
  git add .gitignore deploy/.env.example
  git commit -m "chore: add root .gitignore and deploy/.env.example template"
  ```

---

## Task 4: deploy/mosquitto/mosquitto.conf

**Files:**
- Create: `deploy/mosquitto/mosquitto.conf`

- [ ] **Step 1: Create `deploy/mosquitto/mosquitto.conf`**

  ```
  listener 1883 0.0.0.0
  allow_anonymous false
  password_file /mosquitto/config/passwd

  persistence true
  persistence_location /mosquitto/data/

  log_dest stdout
  log_type error
  log_type warning
  log_type notice
  log_type information
  ```

  Notes:
  - Port 1883 is plain MQTT. It is **not** published to the host in docker-compose — only Nginx can reach it.
  - `allow_anonymous false` requires all clients to authenticate with the passwd file.

- [ ] **Step 2: Commit**

  ```bash
  git add deploy/mosquitto/mosquitto.conf
  git commit -m "feat(deploy): add Mosquitto broker config"
  ```

---

## Task 5: deploy/nginx/nginx.conf.template

**Files:**
- Create: `deploy/nginx/nginx.conf.template`

The template uses `${DOMAIN}` as the only substituted variable. Nginx variables (`$host`, `$remote_addr`, etc.) are left untouched by `envsubst '$DOMAIN'` in docker-compose.

- [ ] **Step 1: Create `deploy/nginx/nginx.conf.template`**

  ```nginx
  events {
      worker_connections 1024;
  }

  http {
      server {
          listen 80;
          server_name ${DOMAIN};

          location /.well-known/acme-challenge/ {
              root /var/www/certbot;
          }

          location / {
              return 301 https://$host$request_uri;
          }
      }

      server {
          listen 443 ssl;
          server_name ${DOMAIN};

          ssl_certificate     /etc/letsencrypt/live/${DOMAIN}/fullchain.pem;
          ssl_certificate_key /etc/letsencrypt/live/${DOMAIN}/privkey.pem;
          ssl_protocols       TLSv1.2 TLSv1.3;
          ssl_ciphers         HIGH:!aNULL:!MD5;

          location / {
              proxy_pass         http://server:3000;
              proxy_set_header   Host              $host;
              proxy_set_header   X-Real-IP         $remote_addr;
              proxy_set_header   X-Forwarded-For   $proxy_add_x_forwarded_for;
              proxy_set_header   X-Forwarded-Proto $scheme;
          }
      }
  }

  stream {
      server {
          listen 8883 ssl;
          ssl_certificate     /etc/letsencrypt/live/${DOMAIN}/fullchain.pem;
          ssl_certificate_key /etc/letsencrypt/live/${DOMAIN}/privkey.pem;
          ssl_protocols       TLSv1.2 TLSv1.3;
          ssl_ciphers         HIGH:!aNULL:!MD5;
          proxy_pass mosquitto:1883;
      }
  }
  ```

- [ ] **Step 2: Commit**

  ```bash
  git add deploy/nginx/nginx.conf.template
  git commit -m "feat(deploy): add Nginx config template for HTTPS + MQTT TLS"
  ```

---

## Task 6: deploy/docker-compose.yml

**Files:**
- Create: `deploy/docker-compose.yml`

- [ ] **Step 1: Create `deploy/docker-compose.yml`**

  ```yaml
  services:
    nginx:
      image: nginx:alpine
      restart: unless-stopped
      ports:
        - "80:80"
        - "443:443"
        - "8883:8883"
      environment:
        - DOMAIN=${DOMAIN}
      command: >-
        /bin/sh -c
        "envsubst '$$DOMAIN' < /etc/nginx/nginx.conf.template > /etc/nginx/nginx.conf
        && exec nginx -g 'daemon off;'"
      volumes:
        - ./nginx/nginx.conf.template:/etc/nginx/nginx.conf.template:ro
        - ./certbot/conf:/etc/letsencrypt:ro
        - ./certbot/www:/var/www/certbot:ro
      depends_on:
        - server
        - mosquitto

    server:
      build:
        context: ../server
        dockerfile: Dockerfile
      restart: unless-stopped
      env_file: .env
      depends_on:
        postgres:
          condition: service_healthy

    postgres:
      image: postgres:15-alpine
      restart: unless-stopped
      environment:
        POSTGRES_USER: ${POSTGRES_USER}
        POSTGRES_PASSWORD: ${POSTGRES_PASSWORD}
        POSTGRES_DB: ${POSTGRES_DB}
      volumes:
        - pg_data:/var/lib/postgresql/data
        - ../server/src/db/schema.sql:/docker-entrypoint-initdb.d/schema.sql:ro
      healthcheck:
        test: ["CMD-SHELL", "pg_isready -U ${POSTGRES_USER} -d ${POSTGRES_DB}"]
        interval: 5s
        timeout: 5s
        retries: 5

    mosquitto:
      image: eclipse-mosquitto:2
      restart: unless-stopped
      volumes:
        - ./mosquitto/mosquitto.conf:/mosquitto/config/mosquitto.conf:ro
        - ./mosquitto/passwd:/mosquitto/config/passwd:ro
        - mosquitto_data:/mosquitto/data
        - mosquitto_log:/mosquitto/log

    certbot:
      image: certbot/certbot
      volumes:
        - ./certbot/conf:/etc/letsencrypt
        - ./certbot/www:/var/www/certbot
      entrypoint: >-
        /bin/sh -c "trap exit TERM;
        while :; do certbot renew --quiet; sleep 12h & wait $${!}; done"

  volumes:
    pg_data:
    mosquitto_data:
    mosquitto_log:
  ```

  Key points:
  - Port 1883 and 5432 are **not** in `ports:` — they are Docker-internal only.
  - `env_file: .env` injects all vars from `deploy/.env` into the `server` container.
  - `schema.sql` is mounted to `/docker-entrypoint-initdb.d/` — postgres runs it automatically the first time the data volume is empty.
  - The certbot service loops every 12 hours to attempt renewal.

- [ ] **Step 2: Validate the compose file (run from `deploy/` directory)**

  ```bash
  cd deploy
  # Create a minimal .env just for validation (do not commit)
  echo "DOMAIN=hung-test.codeaplha.biz
  POSTGRES_USER=loco
  POSTGRES_PASSWORD=test
  POSTGRES_DB=loco_db" > .env.tmp
  docker compose --env-file .env.tmp config
  rm .env.tmp
  cd ..
  ```

  Expected: YAML output with no errors.

- [ ] **Step 3: Commit**

  ```bash
  git add deploy/docker-compose.yml
  git commit -m "feat(deploy): add Docker Compose stack for VPS deployment"
  ```

---

## Task 7: deploy/init-letsencrypt.sh

This script runs **once on the VPS** to bootstrap TLS certs. It generates a temporary self-signed cert so Nginx can start, then uses certbot to replace it with a real Let's Encrypt cert.

**Files:**
- Create: `deploy/init-letsencrypt.sh`

- [ ] **Step 1: Create `deploy/init-letsencrypt.sh`**

  ```bash
  #!/bin/bash
  set -euo pipefail

  SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
  cd "$SCRIPT_DIR"

  if [ ! -f .env ]; then
    echo "ERROR: deploy/.env not found."
    echo "Copy deploy/.env.example to deploy/.env and fill in all values."
    exit 1
  fi

  # Load DOMAIN and CERTBOT_EMAIL from .env
  set -a; source .env; set +a

  CERT_DIR="./certbot/conf/live/${DOMAIN}"

  # Skip if real cert already exists
  if [ -d "$CERT_DIR" ] && [ -f "$CERT_DIR/fullchain.pem" ]; then
    echo "Certs already exist for ${DOMAIN}. To renew, run:"
    echo "  docker compose run --rm certbot renew"
    exit 0
  fi

  # --- Step 1: Generate Mosquitto password file ---
  if [ ! -f "./mosquitto/passwd" ]; then
    echo "==> Generating Mosquitto password file..."
    echo "Enter password for MQTT user 'server' (use value of MQTT_PASS from .env):"
    read -rs SERVER_PASS
    echo "Enter password for MQTT user 'app_user' (use value of APP_MQTT_PASS from .env):"
    read -rs APP_PASS
    docker run --rm -v "${SCRIPT_DIR}/mosquitto:/mosquitto/config" eclipse-mosquitto:2 \
      sh -c "mosquitto_passwd -c -b /mosquitto/config/passwd server '${SERVER_PASS}' && \
             mosquitto_passwd -b /mosquitto/config/passwd app_user '${APP_PASS}'"
    echo "==> Mosquitto passwd file created."
  else
    echo "==> Mosquitto passwd file already exists, skipping."
  fi

  # --- Step 2: Create dummy cert so Nginx can start ---
  echo "==> Creating temporary self-signed cert for ${DOMAIN}..."
  mkdir -p "$CERT_DIR"
  openssl req -x509 -nodes -newkey rsa:2048 -days 1 \
    -keyout "${CERT_DIR}/privkey.pem" \
    -out    "${CERT_DIR}/fullchain.pem" \
    -subj   "/CN=${DOMAIN}" 2>/dev/null

  # --- Step 3: Start only Nginx with dummy cert ---
  echo "==> Starting Nginx with dummy cert..."
  docker compose up --force-recreate -d nginx
  echo "    Waiting 5s for Nginx to be ready..."
  sleep 5

  # --- Step 4: Delete dummy cert, let certbot write the real one ---
  echo "==> Removing dummy cert..."
  rm -rf ./certbot/conf/live

  # --- Step 5: Obtain real Let's Encrypt cert ---
  echo "==> Requesting Let's Encrypt certificate for ${DOMAIN}..."
  docker compose run --rm certbot certonly \
    --webroot -w /var/www/certbot \
    --email "${CERTBOT_EMAIL}" \
    -d "${DOMAIN}" \
    --agree-tos --no-eff-email

  # --- Step 6: Reload Nginx with real cert ---
  echo "==> Reloading Nginx with real certificate..."
  docker compose exec nginx nginx -s reload

  # --- Step 7: Start remaining services ---
  echo "==> Starting all services..."
  docker compose up -d

  # --- Step 8: Create initial admin user ---
  echo ""
  echo "==> Waiting for PostgreSQL to be healthy..."
  until docker compose exec postgres pg_isready -U "${POSTGRES_USER}" -d "${POSTGRES_DB}" -q; do
    sleep 2
  done

  echo ""
  echo "==> Creating initial admin user."
  echo "    Enter username for first admin account:"
  read -r ADMIN_USER
  echo "    Enter password:"
  read -rs ADMIN_PASS

  HASH=$(docker compose exec -T server node -e \
    "const b=require('bcryptjs'); console.log(b.hashSync('${ADMIN_PASS}', 10))")

  docker compose exec -T postgres psql -U "${POSTGRES_USER}" "${POSTGRES_DB}" -c \
    "INSERT INTO users (username, password_hash) VALUES ('${ADMIN_USER}', '${HASH}')
     ON CONFLICT (username) DO NOTHING;"

  echo ""
  echo "============================================"
  echo " Deployment complete!"
  echo " API:  https://${DOMAIN}"
  echo " MQTT: ${DOMAIN}:8883 (TLS)"
  echo "============================================"
  echo ""
  echo "VPS firewall — allow only these ports:"
  echo "  ufw allow 22 && ufw allow 80 && ufw allow 443 && ufw allow 8883"
  echo "  ufw enable"
  ```

- [ ] **Step 2: Make the script executable (on the VPS or via git)**

  ```bash
  chmod +x deploy/init-letsencrypt.sh
  git add deploy/init-letsencrypt.sh
  git update-index --chmod=+x deploy/init-letsencrypt.sh
  ```

- [ ] **Step 3: Commit**

  ```bash
  git commit -m "feat(deploy): add init-letsencrypt.sh bootstrap script"
  ```

---

## Task 8: Update Flutter app server URL

**Files:**
- Modify: `app/lib/providers/auth_provider.dart:4`

- [ ] **Step 1: Update `_serverUrl` in `app/lib/providers/auth_provider.dart`**

  Change line 4:

  ```dart
  // before
  const _serverUrl = 'https://your-vps.com:3000';
  // after
  const _serverUrl = 'https://hung-test.codeaplha.biz';
  ```

  Note: No port in the URL — Nginx terminates TLS on port 443 and proxies to Node.js internally.

- [ ] **Step 2: Commit**

  ```bash
  git add app/lib/providers/auth_provider.dart
  git commit -m "feat(app): set production API URL to hung-test.codeaplha.biz"
  ```

---

## Post-implementation: Deploy to VPS

After all tasks are committed and pushed, run these commands **on the VPS** (`115.146.127.154`):

```bash
# Install Docker (if not already installed)
curl -fsSL https://get.docker.com | sh
sudo usermod -aG docker $USER
newgrp docker

# Clone the repo
git clone <repo-url> ~/locoloco
cd ~/locoloco/WLED

# Create production .env from template
cp deploy/.env.example deploy/.env
nano deploy/.env   # fill in all CHANGE_* values

# Verify DNS: A record for hung-test.codeaplha.biz → 115.146.127.154
# (must propagate before certbot will work)
dig hung-test.codeaplha.biz A

# Run the bootstrap script
cd deploy
bash init-letsencrypt.sh

# Configure firewall
sudo ufw allow 22
sudo ufw allow 80
sudo ufw allow 443
sudo ufw allow 8883
sudo ufw enable
```

---

## Self-Review Notes

- **auth.js bug** is covered by Task 1 with a test that proves the old behavior fails.
- **Schema init** is handled automatically by postgres `/docker-entrypoint-initdb.d/` mount in Task 6 — no manual psql step needed.
- **Mosquitto passwd file** is generated inside init-letsencrypt.sh (Task 7) — no separate step required.
- **Port exposure security**: ports 1883 and 5432 are not in docker-compose `ports:` — they never reach the host.
- **MQTT TLS**: handled entirely by Nginx stream proxy — Mosquitto needs no cert management.
- **Flutter MQTT connection**: login response now returns `APP_MQTT_HOST=hung-test.codeaplha.biz:8883` (Task 1 fix).
