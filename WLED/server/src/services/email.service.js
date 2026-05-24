const nodemailer = require('nodemailer');

const transporter = nodemailer.createTransport({
  host: 'smtp.gmail.com',
  port: 465,
  secure: true,
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
