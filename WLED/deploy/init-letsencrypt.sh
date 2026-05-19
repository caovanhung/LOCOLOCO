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
  docker run --rm \
    -e MQTT_SERVER_PASS="${SERVER_PASS}" \
    -e MQTT_APP_PASS="${APP_PASS}" \
    -v "${SCRIPT_DIR}/mosquitto:/mosquitto/config" eclipse-mosquitto:2 \
    sh -c 'mosquitto_passwd -c -b /mosquitto/config/passwd server "$MQTT_SERVER_PASS" && \
           mosquitto_passwd -b /mosquitto/config/passwd app_user "$MQTT_APP_PASS"'
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

HASH=$(docker compose exec -T -e ADMIN_PASS="${ADMIN_PASS}" server \
  node -e "const b=require('bcryptjs'); console.log(b.hashSync(process.env.ADMIN_PASS, 10))")

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
