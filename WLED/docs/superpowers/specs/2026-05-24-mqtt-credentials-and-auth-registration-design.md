# Design: MQTT Default Credentials + Account Registration & Email Verification

**Date:** 2026-05-24  
**Status:** Approved  
**Scope:** Two independent tasks — (A) fix MQTT credentials, (B) add register + email verify feature

---

## A. MQTT Default Credentials

### Goal
Hardcode consistent MQTT credentials across all three components so the system works out of the box with known values.

### Credentials Table

| User | Password | Used by |
|------|----------|---------|
| `device` | `Loco@device2024` | All ESP8266/ESP32 firmware |
| `server` | `Loco@server2024` | Node.js server → Mosquitto (Docker internal) |
| `app_user` | `Loco@app2024` | Flutter app (returned after login) |

### Files to Update

| File | Change |
|------|--------|
| `SW/LOCO_FIRME_V1.0.3/src/config.h` | `MQTT_PASS "Loco@device2024"` |
| `deploy/.env.example` | Fill `CHANGE_SERVER_MQTT_PASS` and `CHANGE_APP_MQTT_PASS` with real defaults |
| `server/.env.example` | Same |
| `deploy/mosquitto/passwd.example` | Show pre-hashed example lines and generation command |

### Mosquitto Setup (manual step on VPS)
```bash
docker compose exec mosquitto mosquitto_passwd -b /mosquitto/config/passwd device Loco@device2024
docker compose exec mosquitto mosquitto_passwd -b /mosquitto/config/passwd server Loco@server2024
docker compose exec mosquitto mosquitto_passwd -b /mosquitto/config/passwd app_user Loco@app2024
```

---

## B. Account Registration & Email Verification

### Goal
Users must register with username + email + password, receive a verification email, and click a link before they can log in.

### Architecture

```
[Flutter App]                    [Node.js Server]              [Gmail SMTP]
RegisterScreen
  → POST /api/auth/register  →   create user (unverified)
                                  generate token (UUID, 24h TTL)
                             →   send verification email   →   email to user
AwaitVerifyScreen
  (user checks email)

                                  GET /api/auth/verify-email?token=xxx
                                  → validate token + expiry
                                  → set email_verified = true
                                  → return HTML success page

LoginScreen ────────────────→   POST /api/auth/login
                                  → check email_verified = true
                                  → 403 if not verified
                                  → 200 + JWT + MQTT creds if verified
```

### Database Changes

Add 4 columns to the `users` table in `server/src/db/schema.sql`:

```sql
email                TEXT UNIQUE NOT NULL,
email_verified       BOOLEAN NOT NULL DEFAULT false,
verification_token   TEXT,
token_expires_at     TIMESTAMPTZ
```

### Server — New & Modified Files

**`server/src/services/email.service.js`** (new)
- Nodemailer configured with Gmail SMTP
- `sendVerificationEmail(to, username, token)` — sends HTML email with link
- Link format: `https://hung-test.codeaplha.biz/api/auth/verify-email?token=<token>`
- Email expires note: 24 hours

**`server/src/db/queries.js`** (modified)
- `createUser(username, email, passwordHash, token, expiresAt)` — insert unverified user
- `findUserByEmail(email)` — lookup by email
- `findUserByVerificationToken(token)` — lookup for verify endpoint
- `markEmailVerified(userId)` — set email_verified = true, clear token

**`server/src/routes/auth.js`** (modified)
- `POST /api/auth/register` — validate input, check duplicate username/email, hash password, generate token, create user, send email → `201 { message: 'Verification email sent' }`
- `GET /api/auth/verify-email?token` — validate token + expiry → HTML success or error page
- `POST /api/auth/resend-verification` — always returns `200` (no leak), resends if email exists and unverified
- `POST /api/auth/login` — add `email_verified` check → `403 { error: 'Email not verified', resend: true }` if not verified

**New env vars** (add to `server/.env.example` and `deploy/.env.example`):
```
GMAIL_USER=hung.cv.10@gmail.com
GMAIL_APP_PASS=xxxx-xxxx-xxxx-xxxx
APP_BASE_URL=https://hung-test.codeaplha.biz
```

> Gmail requires an **App Password** (not the account password).  
> Enable: Google Account → Security → 2-Step Verification → App passwords.

### Flutter App — New & Modified Files

**`app/lib/screens/register_screen.dart`** (new)
- Fields: Username, Email, Password (with show/hide toggle)
- Calls `ApiService.register()`
- On success: navigate to `AwaitVerifyScreen`, passing email
- On error: show error message (duplicate username/email, etc.)

**`app/lib/screens/await_verify_screen.dart`** (new)
- Shows: "Email xác minh đã gửi đến `<email>`"
- Button: "Gửi lại email" → calls `ApiService.resendVerification(email)`
- Button: "Về trang Login" → pop to LoginScreen

**`app/lib/screens/login_screen.dart`** (modified)
- Add "Chưa có tài khoản? Đăng ký" link → navigate to RegisterScreen
- If login returns 403 with `resend: true`: show snackbar with "Gửi lại email xác minh" action

**`app/lib/services/api_service.dart`** (modified)
- `register(String username, String email, String password)` → `POST /api/auth/register`
- `resendVerification(String email)` → `POST /api/auth/resend-verification`

### Error Handling

| Scenario | Server response | App behaviour |
|----------|----------------|---------------|
| Username already taken | `409 { error: 'Username taken' }` | Show inline error |
| Email already registered | `409 { error: 'Email already registered' }` | Show inline error |
| Token expired | `400 { error: 'Token expired' }` | HTML error page with resend instructions |
| Token invalid | `400 { error: 'Invalid token' }` | HTML error page |
| Login before verify | `403 { error: '...', resend: true }` | Snackbar + resend button |

### Token Design
- Generated with `crypto.randomBytes(32).toString('hex')` — 64-char hex string
- Stored in `verification_token` column
- Expires `NOW() + INTERVAL '24 hours'`
- Cleared after successful verification

---

## Out of Scope
- Password reset / forgot password
- Social login (Google, Facebook)
- Admin panel for user management
- Rate limiting on register/resend beyond existing express-rate-limit setup
