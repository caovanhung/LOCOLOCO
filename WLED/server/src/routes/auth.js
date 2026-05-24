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
