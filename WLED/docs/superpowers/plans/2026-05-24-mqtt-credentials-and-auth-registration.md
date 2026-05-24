# MQTT Credentials + Auth Registration & Email Verification Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix MQTT default credentials across all components, then add user registration with mandatory email verification before login.

**Architecture:** Two work streams. (A) Update hardcoded MQTT passwords in config.h, .env.example, and mosquitto/passwd.example. (B) Extend Node.js server with register/verify/resend endpoints using nodemailer+Gmail SMTP, update Flutter app with RegisterScreen and AwaitVerifyScreen, and update LoginScreen to handle 403 unverified state.

**Tech Stack:** Node.js/Express, PostgreSQL, nodemailer (Gmail SMTP), Flutter/Dart + Riverpod, Mosquitto MQTT broker

---

## File Map

| File | Action | Purpose |
|------|--------|---------|
| `SW/LOCO_FIRME_V1.0.3/src/config.h` | Modify | Update MQTT_PASS |
| `deploy/.env.example` | Modify | Fill MQTT passwords + add Gmail env vars |
| `server/.env.example` | Modify | Same |
| `deploy/mosquitto/passwd.example` | Modify | Update setup commands with real passwords |
| `server/package.json` | Modify | Add nodemailer dependency |
| `server/src/db/schema.sql` | Modify | Add email/verification columns to users table |
| `server/src/db/queries.js` | Modify | Add createUser, findUserByEmail, findUserByVerificationToken, markEmailVerified, updateVerificationToken |
| `server/src/services/email.service.js` | Create | nodemailer Gmail SMTP wrapper |
| `server/src/routes/auth.js` | Modify | Add register, verify-email, resend; update login to check email_verified |
| `app/lib/providers/auth_provider.dart` | Modify | Add isUnverified flag to AuthState |
| `app/lib/services/api_service.dart` | Modify | Add register(), resendVerification() |
| `app/lib/screens/register_screen.dart` | Create | Registration form (username, email, password) |
| `app/lib/screens/await_verify_screen.dart` | Create | "Check your email" screen with resend button |
| `app/lib/screens/login_screen.dart` | Modify | Add Register link + resend dialog on 403 |

---

## Task 1: MQTT Default Credentials

**Files:**
- Modify: `SW/LOCO_FIRME_V1.0.3/src/config.h`
- Modify: `deploy/.env.example`
- Modify: `server/.env.example`
- Modify: `deploy/mosquitto/passwd.example`

- [ ] **Step 1: Update ESP firmware password in config.h**

Change line 20 in `SW/LOCO_FIRME_V1.0.3/src/config.h`:
```c
#define MQTT_PASS         "Loco@device2024"  // must match password set in deploy/mosquitto/passwd
```

- [ ] **Step 2: Update deploy/.env.example**

Replace the MQTT and Gmail sections:
```env
# Internal server → Mosquitto (Docker network, no TLS needed)
MQTT_HOST=mosquitto
MQTT_PORT=1883
MQTT_USER=server
MQTT_PASS=Loco@server2024

# Public MQTT endpoint returned to Flutter app after login
# Port 1883 — no TLS (Mosquitto does not have a TLS listener configured)
APP_MQTT_HOST=hung-test.codeaplha.biz
APP_MQTT_PORT=1883
APP_MQTT_USER=app_user
APP_MQTT_PASS=Loco@app2024

# Gmail SMTP for email verification
GMAIL_USER=hung.cv.10@gmail.com
GMAIL_APP_PASS=xxxx-xxxx-xxxx-xxxx

# Base URL for verification links sent in emails
APP_BASE_URL=https://hung-test.codeaplha.biz
```

- [ ] **Step 3: Replace server/.env.example**

Full file content:
```env
PORT=3000
DATABASE_URL=postgres://loco:CHANGE_DB_PASS@postgres:5432/loco_db
JWT_SECRET=change-this-to-random-64-char-string
JWT_EXPIRES_IN=7d

# Server → Mosquitto (Docker internal network, no TLS)
MQTT_HOST=mosquitto
MQTT_PORT=1883
MQTT_USER=server
MQTT_PASS=Loco@server2024
MQTT_USE_TLS=false

# Public MQTT endpoint returned to Flutter app after login
APP_MQTT_HOST=hung-test.codeaplha.biz
APP_MQTT_PORT=1883
APP_MQTT_USER=app_user
APP_MQTT_PASS=Loco@app2024

# Gmail SMTP for email verification
GMAIL_USER=hung.cv.10@gmail.com
GMAIL_APP_PASS=xxxx-xxxx-xxxx-xxxx

# Base URL for verification links sent in emails
APP_BASE_URL=https://hung-test.codeaplha.biz
```

- [ ] **Step 4: Update deploy/mosquitto/passwd.example**

Full file content:
```
# Mosquitto password file — DO NOT commit the real passwd file to git.
#
# Create the real passwd file on VPS (run once, then restart mosquitto):
#
#   docker compose exec mosquitto mosquitto_passwd -b /mosquitto/config/passwd device   Loco@device2024
#   docker compose exec mosquitto mosquitto_passwd -b /mosquitto/config/passwd server   Loco@server2024
#   docker compose exec mosquitto mosquitto_passwd -b /mosquitto/config/passwd app_user Loco@app2024
#   docker compose restart mosquitto
#
# Three users:
#   device   — all ESP8266/ESP32 firmware  (config.h: MQTT_USER / MQTT_PASS)
#   server   — Node.js server              (.env: MQTT_USER / MQTT_PASS)
#   app_user — Flutter app after login     (.env: APP_MQTT_USER / APP_MQTT_PASS)
```

- [ ] **Step 5: Commit**

```bash
git add SW/LOCO_FIRME_V1.0.3/src/config.h deploy/.env.example server/.env.example deploy/mosquitto/passwd.example
git commit -m "config: set default MQTT credentials across ESP, server, and broker"
```

---

## Task 2: Install nodemailer

**Files:**
- Modify: `server/package.json`

- [ ] **Step 1: Install nodemailer**

```bash
cd server && npm install nodemailer
```

- [ ] **Step 2: Verify**

```bash
node -e "require('nodemailer'); console.log('ok')"
```
Expected output: `ok`

- [ ] **Step 3: Commit**

```bash
git add server/package.json server/package-lock.json
git commit -m "chore(server): add nodemailer for Gmail SMTP"
```

---

## Task 3: DB Schema — Add Email Verification Columns

**Files:**
- Modify: `server/src/db/schema.sql`

- [ ] **Step 1: Replace users CREATE TABLE block**

In `server/src/db/schema.sql`, replace:
```sql
CREATE TABLE IF NOT EXISTS users (
  id            SERIAL PRIMARY KEY,
  username      VARCHAR(50) UNIQUE NOT NULL,
  password_hash VARCHAR(255) NOT NULL
);
```

With:
```sql
CREATE TABLE IF NOT EXISTS users (
  id                 SERIAL PRIMARY KEY,
  username           VARCHAR(50) UNIQUE NOT NULL,
  password_hash      VARCHAR(255) NOT NULL,
  email              TEXT UNIQUE NOT NULL,
  email_verified     BOOLEAN NOT NULL DEFAULT false,
  verification_token TEXT,
  token_expires_at   TIMESTAMPTZ
);
```

- [ ] **Step 2: Migration SQL for existing VPS DB**

If the VPS DB volume already exists, run this on the VPS after deploying:
```bash
docker compose exec postgres psql -U loco -d loco_db -c "
  ALTER TABLE users ADD COLUMN IF NOT EXISTS email TEXT UNIQUE NOT NULL DEFAULT '';
  ALTER TABLE users ADD COLUMN IF NOT EXISTS email_verified BOOLEAN NOT NULL DEFAULT false;
  ALTER TABLE users ADD COLUMN IF NOT EXISTS verification_token TEXT;
  ALTER TABLE users ADD COLUMN IF NOT EXISTS token_expires_at TIMESTAMPTZ;
"
```
> Fresh deployments (new volume) use schema.sql automatically — no migration needed.

- [ ] **Step 3: Commit**

```bash
git add server/src/db/schema.sql
git commit -m "feat(db): add email verification columns to users table"
```

---

## Task 4: DB Queries — Registration & Verification

**Files:**
- Modify: `server/src/db/queries.js`

- [ ] **Step 1: Add 5 new query functions before the `end:` line**

In `server/src/db/queries.js`, add before `end: () => pool.end(),`:
```js
  createUser: (username, email, passwordHash, token, expiresAt) =>
    pool.query(
      `INSERT INTO users (username, email, password_hash, verification_token, token_expires_at)
       VALUES ($1, $2, $3, $4, $5) RETURNING id, username, email`,
      [username, email, passwordHash, token, expiresAt]
    ).then(r => r.rows[0]),

  findUserByEmail: (email) =>
    pool.query('SELECT * FROM users WHERE email = $1', [email])
      .then(r => r.rows[0]),

  findUserByVerificationToken: (token) =>
    pool.query('SELECT * FROM users WHERE verification_token = $1', [token])
      .then(r => r.rows[0]),

  markEmailVerified: (userId) =>
    pool.query(
      `UPDATE users SET email_verified = true, verification_token = NULL, token_expires_at = NULL
       WHERE id = $1`,
      [userId]
    ),

  updateVerificationToken: (userId, token, expiresAt) =>
    pool.query(
      'UPDATE users SET verification_token = $2, token_expires_at = $3 WHERE id = $1',
      [userId, token, expiresAt]
    ),
```

- [ ] **Step 2: Verify syntax**

```bash
cd server && node -e "const db = require('./src/db/queries'); console.log(Object.keys(db).join(', '))"
```
Expected output includes: `createUser, findUserByEmail, findUserByVerificationToken, markEmailVerified, updateVerificationToken`

- [ ] **Step 3: Commit**

```bash
git add server/src/db/queries.js
git commit -m "feat(db): add user registration and email verification queries"
```

---

## Task 5: Email Service

**Files:**
- Create: `server/src/services/email.service.js`

- [ ] **Step 1: Create the email service**

Create `server/src/services/email.service.js`:
```js
const nodemailer = require('nodemailer');

const transporter = nodemailer.createTransport({
  service: 'gmail',
  auth: {
    user: process.env.GMAIL_USER,
    pass: process.env.GMAIL_APP_PASS,
  },
});

async function sendVerificationEmail(to, username, token) {
  const link = `${process.env.APP_BASE_URL}/api/auth/verify-email?token=${token}`;
  await transporter.sendMail({
    from: `"LOCOLOCO" <${process.env.GMAIL_USER}>`,
    to,
    subject: '[LOCOLOCO] Xác minh tài khoản',
    html: `
      <p>Chào <b>${username}</b>,</p>
      <p>Click link bên dưới để xác minh tài khoản (hết hạn sau 24 giờ):</p>
      <p><a href="${link}">${link}</a></p>
      <p>Nếu bạn không đăng ký, bỏ qua email này.</p>
    `,
  });
}

module.exports = { sendVerificationEmail };
```

- [ ] **Step 2: Verify syntax**

```bash
cd server && node -e "require('./src/services/email.service'); console.log('ok')"
```
Expected output: `ok`

- [ ] **Step 3: Commit**

```bash
git add server/src/services/email.service.js
git commit -m "feat(server): add Gmail SMTP email service"
```

---

## Task 6: Auth Routes — Register, Verify, Resend, Update Login

**Files:**
- Modify: `server/src/routes/auth.js`

- [ ] **Step 1: Replace entire auth.js**

```js
const router = require('express').Router();
const bcrypt = require('bcryptjs');
const jwt = require('jsonwebtoken');
const crypto = require('crypto');
const rateLimit = require('express-rate-limit');
const db = require('../db/queries');
const { sendVerificationEmail } = require('../services/email.service');

const loginLimiter    = rateLimit({ windowMs: 15 * 60 * 1000, max: 20 });
const registerLimiter = rateLimit({ windowMs: 60 * 60 * 1000, max: 10 });

// POST /api/auth/register
router.post('/register', registerLimiter, async (req, res) => {
  const { username, password, email } = req.body;
  if (!username || !password || !email)
    return res.status(400).json({ error: 'username, email and password required' });
  if (!/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(email))
    return res.status(400).json({ error: 'Invalid email format' });
  if (username.length < 3 || username.length > 50)
    return res.status(400).json({ error: 'Username must be 3–50 characters' });
  if (password.length < 6)
    return res.status(400).json({ error: 'Password must be at least 6 characters' });

  try {
    const existingUser = await db.findUserByUsername(username);
    if (existingUser) return res.status(409).json({ error: 'Username already taken' });

    const existingEmail = await db.findUserByEmail(email);
    if (existingEmail) return res.status(409).json({ error: 'Email already registered' });

    const passwordHash = await bcrypt.hash(password, 10);
    const token = crypto.randomBytes(32).toString('hex');
    const expiresAt = new Date(Date.now() + 24 * 60 * 60 * 1000);

    await db.createUser(username, email, passwordHash, token, expiresAt);
    await sendVerificationEmail(email, username, token);

    res.status(201).json({ message: 'Verification email sent. Please check your inbox.' });
  } catch (err) {
    console.error('register error', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

// GET /api/auth/verify-email?token=xxx
router.get('/verify-email', async (req, res) => {
  const { token } = req.query;
  if (!token) return res.status(400).send(htmlPage('Lỗi', 'Token không hợp lệ.'));

  try {
    const user = await db.findUserByVerificationToken(token);
    if (!user)
      return res.status(400).send(htmlPage('Lỗi', 'Token không hợp lệ hoặc đã được dùng.'));
    if (new Date() > new Date(user.token_expires_at))
      return res.status(400).send(htmlPage('Hết hạn', 'Link đã hết hạn. Hãy đăng nhập rồi chọn "Gửi lại email xác minh".'));

    await db.markEmailVerified(user.id);
    res.send(htmlPage('Thành công!', 'Email đã được xác minh. Bạn có thể đăng nhập vào ứng dụng.'));
  } catch (err) {
    console.error('verify-email error', err);
    res.status(500).send(htmlPage('Lỗi', 'Lỗi server. Vui lòng thử lại.'));
  }
});

// POST /api/auth/resend-verification
router.post('/resend-verification', loginLimiter, async (req, res) => {
  // Always return 200 to avoid email enumeration
  res.json({ message: 'If this email exists and is unverified, a new link has been sent.' });

  const { email } = req.body;
  if (!email) return;
  try {
    const user = await db.findUserByEmail(email);
    if (!user || user.email_verified) return;

    const token = crypto.randomBytes(32).toString('hex');
    const expiresAt = new Date(Date.now() + 24 * 60 * 60 * 1000);
    await db.updateVerificationToken(user.id, token, expiresAt);
    await sendVerificationEmail(user.email, user.username, token);
  } catch (err) {
    console.error('resend-verification error', err);
  }
});

// POST /api/auth/login
router.post('/login', loginLimiter, async (req, res) => {
  const { username, password } = req.body;
  if (!username || !password)
    return res.status(400).json({ error: 'username and password required' });

  try {
    const user = await db.findUserByUsername(username);
    if (!user) return res.status(401).json({ error: 'Invalid credentials' });

    const ok = await bcrypt.compare(password, user.password_hash);
    if (!ok) return res.status(401).json({ error: 'Invalid credentials' });

    if (!user.email_verified)
      return res.status(403).json({
        error: 'Email chưa được xác minh. Kiểm tra hộp thư của bạn.',
        resend: true,
      });

    const token = jwt.sign(
      { id: user.id, username: user.username },
      process.env.JWT_SECRET,
      { expiresIn: process.env.JWT_EXPIRES_IN || '7d' }
    );

    res.json({
      token,
      mqtt: {
        host:     process.env.APP_MQTT_HOST,
        port:     Number(process.env.APP_MQTT_PORT) || 1883,
        username: process.env.APP_MQTT_USER,
        password: process.env.APP_MQTT_PASS,
      },
    });
  } catch (err) {
    console.error('login error', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

function htmlPage(title, message) {
  return `<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <title>${title} — LOCOLOCO</title>
  <style>
    body { font-family: sans-serif; display: flex; justify-content: center;
           align-items: center; height: 100vh; margin: 0; background: #f5f5f5; }
    .box { background: #fff; padding: 40px; border-radius: 8px;
           box-shadow: 0 2px 8px rgba(0,0,0,.1); text-align: center; max-width: 400px; }
  </style>
</head>
<body>
  <div class="box"><h2>${title}</h2><p>${message}</p></div>
</body>
</html>`;
}

module.exports = router;
```

- [ ] **Step 2: Verify syntax**

```bash
cd server && node -e "require('./src/routes/auth'); console.log('ok')"
```
Expected: `ok`

- [ ] **Step 3: Commit**

```bash
git add server/src/routes/auth.js
git commit -m "feat(server): add register, verify-email, resend-verification; login checks email_verified"
```

---

## Task 7: Flutter — AuthState isUnverified Flag

**Files:**
- Modify: `app/lib/providers/auth_provider.dart`

- [ ] **Step 1: Replace auth_provider.dart**

```dart
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../services/api_service.dart';

const _serverUrl = 'https://hung-test.codeaplha.biz';

final apiServiceProvider = Provider<ApiService>((ref) => ApiService(_serverUrl));

class AuthState {
  final bool isLoading;
  final String? token;
  final MqttCredentials? mqttCreds;
  final String? error;
  final bool isUnverified;

  const AuthState({
    this.isLoading = false,
    this.token,
    this.mqttCreds,
    this.error,
    this.isUnverified = false,
  });

  bool get isLoggedIn => token != null;

  AuthState copyWith({
    bool? isLoading,
    String? token,
    MqttCredentials? mqttCreds,
    String? error,
    bool? isUnverified,
  }) => AuthState(
    isLoading: isLoading ?? this.isLoading,
    token: token ?? this.token,
    mqttCreds: mqttCreds ?? this.mqttCreds,
    error: error,
    isUnverified: isUnverified ?? false,
  );
}

class AuthNotifier extends StateNotifier<AuthState> {
  final ApiService _api;
  AuthNotifier(this._api) : super(const AuthState());

  Future<void> login(String username, String password) async {
    state = state.copyWith(isLoading: true, error: null, isUnverified: false);
    try {
      final result = await _api.login(username, password);
      _api.setToken(result.token);
      state = state.copyWith(isLoading: false, token: result.token, mqttCreds: result.mqtt);
    } on ApiException catch (e) {
      state = state.copyWith(
        isLoading: false,
        error: e.message,
        isUnverified: e.statusCode == 403,
      );
    } catch (e) {
      state = state.copyWith(isLoading: false, error: e.toString());
    }
  }

  void logout() {
    state = const AuthState();
  }
}

final authProvider = StateNotifierProvider<AuthNotifier, AuthState>((ref) {
  return AuthNotifier(ref.watch(apiServiceProvider));
});
```

- [ ] **Step 2: Verify no analysis errors**

```bash
cd app && flutter analyze lib/providers/auth_provider.dart
```
Expected: `No issues found!`

- [ ] **Step 3: Commit**

```bash
git add app/lib/providers/auth_provider.dart
git commit -m "feat(app): add isUnverified flag to AuthState for 403 login response"
```

---

## Task 8: Flutter ApiService — register() and resendVerification()

**Files:**
- Modify: `app/lib/services/api_service.dart`

- [ ] **Step 1: Add two methods after login()**

In `app/lib/services/api_service.dart`, add after the `login()` method:
```dart
  Future<void> register(String username, String email, String password) => _call(() async {
    await _dio.post('/api/auth/register',
        data: {'username': username, 'email': email, 'password': password});
  });

  Future<void> resendVerification(String email) => _call(() async {
    await _dio.post('/api/auth/resend-verification', data: {'email': email});
  });
```

- [ ] **Step 2: Verify no analysis errors**

```bash
cd app && flutter analyze lib/services/api_service.dart
```
Expected: `No issues found!`

- [ ] **Step 3: Commit**

```bash
git add app/lib/services/api_service.dart
git commit -m "feat(app): add register and resendVerification to ApiService"
```

---

## Task 9: Flutter — AwaitVerifyScreen

**Files:**
- Create: `app/lib/screens/await_verify_screen.dart`

- [ ] **Step 1: Create AwaitVerifyScreen**

Create `app/lib/screens/await_verify_screen.dart`:
```dart
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../providers/auth_provider.dart';
import '../services/api_service.dart';

class AwaitVerifyScreen extends ConsumerStatefulWidget {
  final String email;
  const AwaitVerifyScreen({super.key, required this.email});
  @override
  ConsumerState<AwaitVerifyScreen> createState() => _AwaitVerifyScreenState();
}

class _AwaitVerifyScreenState extends ConsumerState<AwaitVerifyScreen> {
  bool _isSending = false;

  Future<void> _resend() async {
    setState(() { _isSending = true; });
    try {
      final api = ref.read(apiServiceProvider);
      await api.resendVerification(widget.email);
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('Email xác minh đã được gửi lại.')));
      }
    } catch (_) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('Không thể gửi. Thử lại sau.')));
      }
    } finally {
      if (mounted) setState(() { _isSending = false; });
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Kiểm tra email')),
      body: Padding(
        padding: const EdgeInsets.all(24),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            const Icon(Icons.email_outlined, size: 72, color: Colors.blue),
            const SizedBox(height: 24),
            const Text('Xác minh tài khoản',
                style: TextStyle(fontSize: 22, fontWeight: FontWeight.bold)),
            const SizedBox(height: 12),
            Text(
              'Email xác minh đã gửi đến:\n${widget.email}',
              textAlign: TextAlign.center,
              style: const TextStyle(fontSize: 15),
            ),
            const SizedBox(height: 8),
            const Text(
              'Click link trong email để kích hoạt tài khoản.\nLink có hiệu lực trong 24 giờ.',
              textAlign: TextAlign.center,
              style: TextStyle(color: Colors.grey),
            ),
            const SizedBox(height: 32),
            ElevatedButton(
              onPressed: _isSending ? null : _resend,
              child: _isSending
                  ? const SizedBox(width: 20, height: 20,
                      child: CircularProgressIndicator(strokeWidth: 2))
                  : const Text('Gửi lại email'),
            ),
            const SizedBox(height: 12),
            TextButton(
              onPressed: () => Navigator.of(context).popUntil((r) => r.isFirst),
              child: const Text('Về trang Đăng nhập'),
            ),
          ],
        ),
      ),
    );
  }
}
```

- [ ] **Step 2: Verify no analysis errors**

```bash
cd app && flutter analyze lib/screens/await_verify_screen.dart
```
Expected: `No issues found!`

- [ ] **Step 3: Commit**

```bash
git add app/lib/screens/await_verify_screen.dart
git commit -m "feat(app): add AwaitVerifyScreen"
```

---

## Task 10: Flutter — RegisterScreen

**Files:**
- Create: `app/lib/screens/register_screen.dart`

- [ ] **Step 1: Create RegisterScreen**

Create `app/lib/screens/register_screen.dart`:
```dart
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../providers/auth_provider.dart';
import '../services/api_service.dart';
import 'await_verify_screen.dart';

class RegisterScreen extends ConsumerStatefulWidget {
  const RegisterScreen({super.key});
  @override
  ConsumerState<RegisterScreen> createState() => _RegisterScreenState();
}

class _RegisterScreenState extends ConsumerState<RegisterScreen> {
  final _userCtrl  = TextEditingController();
  final _emailCtrl = TextEditingController();
  final _passCtrl  = TextEditingController();
  bool _isLoading = false;
  String? _error;

  @override
  void dispose() {
    _userCtrl.dispose();
    _emailCtrl.dispose();
    _passCtrl.dispose();
    super.dispose();
  }

  Future<void> _register() async {
    setState(() { _isLoading = true; _error = null; });
    try {
      final api = ref.read(apiServiceProvider);
      await api.register(
        _userCtrl.text.trim(),
        _emailCtrl.text.trim(),
        _passCtrl.text,
      );
      if (mounted) {
        Navigator.of(context).pushReplacement(MaterialPageRoute(
          builder: (_) => AwaitVerifyScreen(email: _emailCtrl.text.trim()),
        ));
      }
    } on ApiException catch (e) {
      setState(() { _error = e.message; });
    } catch (_) {
      setState(() { _error = 'Lỗi kết nối. Thử lại sau.'; });
    } finally {
      if (mounted) setState(() { _isLoading = false; });
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Đăng ký')),
      body: Padding(
        padding: const EdgeInsets.all(24),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            TextField(
              controller: _userCtrl,
              decoration: const InputDecoration(labelText: 'Username'),
            ),
            const SizedBox(height: 12),
            TextField(
              controller: _emailCtrl,
              keyboardType: TextInputType.emailAddress,
              decoration: const InputDecoration(labelText: 'Email'),
            ),
            const SizedBox(height: 12),
            TextField(
              controller: _passCtrl,
              obscureText: true,
              decoration: const InputDecoration(labelText: 'Password'),
            ),
            const SizedBox(height: 24),
            if (_error != null)
              Padding(
                padding: const EdgeInsets.only(bottom: 12),
                child: Text(_error!, style: const TextStyle(color: Colors.red)),
              ),
            ElevatedButton(
              onPressed: _isLoading ? null : _register,
              child: _isLoading
                  ? const SizedBox(width: 20, height: 20,
                      child: CircularProgressIndicator(strokeWidth: 2))
                  : const Text('Đăng ký'),
            ),
            TextButton(
              onPressed: () => Navigator.of(context).pop(),
              child: const Text('Đã có tài khoản? Đăng nhập'),
            ),
          ],
        ),
      ),
    );
  }
}
```

- [ ] **Step 2: Verify no analysis errors**

```bash
cd app && flutter analyze lib/screens/register_screen.dart
```
Expected: `No issues found!`

- [ ] **Step 3: Commit**

```bash
git add app/lib/screens/register_screen.dart
git commit -m "feat(app): add RegisterScreen"
```

---

## Task 11: Flutter — Update LoginScreen

**Files:**
- Modify: `app/lib/screens/login_screen.dart`

- [ ] **Step 1: Replace login_screen.dart**

```dart
import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../providers/auth_provider.dart';
import '../providers/mqtt_provider.dart';
import '../services/api_service.dart';
import 'device_list_screen.dart';
import 'register_screen.dart';

class LoginScreen extends ConsumerStatefulWidget {
  const LoginScreen({super.key});
  @override
  ConsumerState<LoginScreen> createState() => _LoginScreenState();
}

class _LoginScreenState extends ConsumerState<LoginScreen> {
  final _userCtrl = TextEditingController();
  final _passCtrl = TextEditingController();

  Future<void> _login() async {
    debugPrint('[LOGIN] starting login');
    await ref.read(authProvider.notifier).login(_userCtrl.text, _passCtrl.text);
    final auth = ref.read(authProvider);

    if (!mounted) return;

    if (auth.isUnverified) {
      _showResendDialog();
      return;
    }

    if (auth.isLoggedIn && auth.mqttCreds != null) {
      final mqtt = ref.read(mqttServiceProvider);
      try {
        debugPrint('[LOGIN] connecting MQTT to ${auth.mqttCreds!.host}:${auth.mqttCreds!.port}');
        await mqtt.connect(
          host: auth.mqttCreds!.host,
          port: auth.mqttCreds!.port,
          username: auth.mqttCreds!.username,
          password: auth.mqttCreds!.password,
        );
        debugPrint('[LOGIN] MQTT connected=${mqtt.isConnected}');
      } catch (e) {
        debugPrint('[LOGIN] MQTT error: $e');
      }
      ref.read(mqttConnectedProvider.notifier).state = mqtt.isConnected;
      if (mounted) {
        Navigator.of(context).pushReplacement(
          MaterialPageRoute(builder: (_) => const DeviceListScreen()));
      }
    }
  }

  void _showResendDialog() {
    final emailCtrl = TextEditingController();
    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('Email chưa xác minh'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            const Text('Nhập email để nhận lại link xác minh:'),
            const SizedBox(height: 12),
            TextField(
              controller: emailCtrl,
              keyboardType: TextInputType.emailAddress,
              decoration: const InputDecoration(labelText: 'Email', border: OutlineInputBorder()),
            ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx),
            child: const Text('Hủy'),
          ),
          TextButton(
            onPressed: () async {
              Navigator.pop(ctx);
              try {
                final api = ref.read(apiServiceProvider);
                await api.resendVerification(emailCtrl.text.trim());
                if (mounted) {
                  ScaffoldMessenger.of(context).showSnackBar(
                    const SnackBar(content: Text('Email xác minh đã được gửi lại.')));
                }
              } catch (_) {
                if (mounted) {
                  ScaffoldMessenger.of(context).showSnackBar(
                    const SnackBar(content: Text('Không thể gửi. Thử lại sau.')));
                }
              }
            },
            child: const Text('Gửi lại'),
          ),
        ],
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    final auth = ref.watch(authProvider);
    return Scaffold(
      appBar: AppBar(title: const Text('LOCOLOCO')),
      body: Padding(
        padding: const EdgeInsets.all(24),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            TextField(
              controller: _userCtrl,
              decoration: const InputDecoration(labelText: 'Username'),
            ),
            const SizedBox(height: 12),
            TextField(
              controller: _passCtrl,
              obscureText: true,
              decoration: const InputDecoration(labelText: 'Password'),
            ),
            const SizedBox(height: 24),
            if (auth.error != null && !auth.isUnverified)
              Text(auth.error!, style: const TextStyle(color: Colors.red)),
            ElevatedButton(
              onPressed: auth.isLoading ? null : _login,
              child: auth.isLoading
                  ? const SizedBox(width: 20, height: 20,
                      child: CircularProgressIndicator(strokeWidth: 2))
                  : const Text('Đăng nhập'),
            ),
            const SizedBox(height: 12),
            TextButton(
              onPressed: () => Navigator.of(context).push(
                MaterialPageRoute(builder: (_) => const RegisterScreen())),
              child: const Text('Chưa có tài khoản? Đăng ký'),
            ),
          ],
        ),
      ),
    );
  }
}
```

- [ ] **Step 2: Verify no analysis errors**

```bash
cd app && flutter analyze lib/screens/login_screen.dart
```
Expected: `No issues found!`

- [ ] **Step 3: Full app analysis**

```bash
cd app && flutter analyze
```
Expected: `No issues found!`

- [ ] **Step 4: Commit**

```bash
git add app/lib/screens/login_screen.dart
git commit -m "feat(app): update LoginScreen with register link and resend dialog on unverified login"
```

---

## After All Tasks: End-to-End Checklist (Manual, on VPS)

- [ ] Copy `.env.example` to `.env`, fill in `GMAIL_APP_PASS` (get from Google Account → Security → 2-Step Verification → App passwords), set real `JWT_SECRET` and `DATABASE_URL` password
- [ ] Deploy: `docker compose pull && docker compose up -d`
- [ ] Run migration SQL if DB volume pre-exists (see Task 3, Step 2)
- [ ] Create Mosquitto users (see Task 1, Step 4)
- [ ] Test full flow: Register → check email → click link → login successfully
- [ ] Test unverified login: shows resend dialog
- [ ] Test resend: new email arrives with new valid link
