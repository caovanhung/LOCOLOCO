const { Pool } = require('pg');
const pool = new Pool({ connectionString: process.env.DATABASE_URL });

module.exports = {
  findUserByUsername: (username) =>
    pool.query('SELECT * FROM users WHERE username = $1', [username])
      .then(r => r.rows[0]),

  getAllDevices: () =>
    pool.query('SELECT id, name, location, online, created_at FROM devices ORDER BY created_at DESC')
      .then(r => r.rows),

  // Option A: auto-register device from MQTT provision message, no-op if already exists
  provisionDevice: (id, name) =>
    pool.query(
      `INSERT INTO devices (id, name) VALUES ($1, $2) ON CONFLICT (id) DO NOTHING RETURNING *`,
      [id, name]
    ).then(r => r.rows[0]),

  upsertDevice: (id, name, location) =>
    pool.query(
      `INSERT INTO devices (id, name, location)
       VALUES ($1, $2, $3)
       ON CONFLICT (id) DO UPDATE SET name=$2, location=$3
       RETURNING *`,
      [id, name, location]
    ).then(r => r.rows[0]),

  deleteDevice: (id) =>
    pool.query('DELETE FROM devices WHERE id = $1', [id]),

  updateDeviceOnline: (id, online) =>
    pool.query('UPDATE devices SET online=$2 WHERE id=$1', [id, online]),

  updateDeviceState: (id, lastState) =>
    pool.query('UPDATE devices SET last_state=$2 WHERE id=$1', [id, lastState]),

  getSchedulesByDevice: (deviceId) =>
    pool.query('SELECT * FROM schedules WHERE device_id=$1 ORDER BY id', [deviceId])
      .then(r => r.rows),

  getAllEnabledSchedules: () =>
    pool.query('SELECT * FROM schedules WHERE enabled=true')
      .then(r => r.rows),

  createSchedule: (deviceId, cronExpr, command) =>
    pool.query(
      'INSERT INTO schedules (device_id, cron_expr, command) VALUES ($1,$2,$3) RETURNING *',
      [deviceId, cronExpr, command]
    ).then(r => r.rows[0]),

  updateSchedule: (id, cronExpr, command, enabled) =>
    pool.query(
      'UPDATE schedules SET cron_expr=$2, command=$3, enabled=$4 WHERE id=$1 RETURNING *',
      [id, cronExpr, command, enabled]
    ).then(r => r.rows[0]),

  deleteSchedule: (id) =>
    pool.query('DELETE FROM schedules WHERE id=$1', [id]),

  getRulesByDevice: (deviceId) =>
    pool.query('SELECT * FROM sensor_rules WHERE device_id=$1 ORDER BY id', [deviceId])
      .then(r => r.rows),

  getEnabledRulesByDevice: (deviceId) =>
    pool.query('SELECT * FROM sensor_rules WHERE device_id=$1 AND enabled=true', [deviceId])
      .then(r => r.rows),

  createRule: (deviceId, sensorType, conditionOp, conditionVal, command) =>
    pool.query(
      `INSERT INTO sensor_rules (device_id, sensor_type, condition_op, condition_val, command)
       VALUES ($1,$2,$3,$4,$5) RETURNING *`,
      [deviceId, sensorType, conditionOp, conditionVal, command]
    ).then(r => r.rows[0]),

  updateRule: (id, sensorType, conditionOp, conditionVal, command, enabled) =>
    pool.query(
      `UPDATE sensor_rules SET sensor_type=$2, condition_op=$3, condition_val=$4,
       command=$5, enabled=$6 WHERE id=$1 RETURNING *`,
      [id, sensorType, conditionOp, conditionVal, command, enabled]
    ).then(r => r.rows[0]),

  deleteRule: (id) =>
    pool.query('DELETE FROM sensor_rules WHERE id=$1', [id]),

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

  end: () => pool.end(),
};
