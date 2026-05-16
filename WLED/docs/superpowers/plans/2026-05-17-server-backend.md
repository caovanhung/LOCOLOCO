# Server Backend Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Xây dựng Node.js server với REST API + MQTT bridge + scheduler + sensor rule engine, chạy trên VPS cùng Mosquitto broker và PostgreSQL.

**Architecture:** Express app expose HTTPS REST API. MQTT service kết nối Mosquitto, subscribe sensor/event topics, route tới rule.service và persist state vào DB. node-cron scheduler load jobs từ DB, publish LED commands đúng giờ. JWT auth cho tất cả REST endpoints.

**Tech Stack:** Node.js 20 LTS, Express 4, PostgreSQL 15, mqtt.js 5, node-cron 3, jsonwebtoken, bcryptjs, pg, dotenv, Jest + Supertest (test)

---

## File Structure

```
server/
  .env.example
  package.json
  src/
    index.js                   — Express app + MQTT init, server start
    db/
      schema.sql               — CREATE TABLE statements
      queries.js               — tất cả DB queries (không dùng ORM)
    middleware/
      auth.js                  — JWT verify middleware
    routes/
      auth.js                  — POST /api/auth/login
      devices.js               — GET/POST/DELETE /api/devices
      schedules.js             — CRUD /api/schedules
      rules.js                 — CRUD /api/rules
    services/
      mqtt.service.js          — MQTT client, connect, pub/sub routing
      scheduler.service.js     — node-cron load/reload jobs
      rule.service.js          — evaluate sensor rule → publish command
  tests/
    auth.test.js
    devices.test.js
    rule.service.test.js
    scheduler.service.test.js
```

---

### Task 1: Scaffold Node.js project

**Files:**
- Create: `server/package.json`, `server/.env.example`, `server/src/index.js`

- [ ] **Step 1: Init project**

```bash
mkdir server && cd server
npm init -y
npm install express pg mqtt jsonwebtoken bcryptjs node-cron dotenv
npm install --save-dev jest supertest
```

- [ ] **Step 2: Cập nhật package.json scripts**

```json
{
  "scripts": {
    "start": "node src/index.js",
    "dev": "node --watch src/index.js",
    "test": "jest --runInBand"
  },
  "jest": {
    "testEnvironment": "node"
  }
}
```

- [ ] **Step 3: Tạo .env.example**

```bash
# server/.env.example
PORT=3000
DATABASE_URL=postgres://loco:password@localhost:5432/loco_db
JWT_SECRET=change-this-to-random-64-char-string
JWT_EXPIRES_IN=7d

MQTT_HOST=localhost
MQTT_PORT=8883
MQTT_USER=server
MQTT_PASS=server_password
MQTT_USE_TLS=true

APP_MQTT_USER=app_user
APP_MQTT_PASS=app_password
```

Tạo `.env` bằng cách copy `.env.example` và điền giá trị thực.

- [ ] **Step 4: Tạo src/index.js tạm**

```js
// server/src/index.js
require('dotenv').config();
const express = require('express');
const app = express();
app.use(express.json());
app.get('/health', (_, res) => res.json({ ok: true }));
const PORT = process.env.PORT || 3000;
app.listen(PORT, () => console.log(`Server running on port ${PORT}`));
module.exports = app;
```

- [ ] **Step 5: Verify server starts**

```bash
node src/index.js
# curl http://localhost:3000/health
```
Expected: `{"ok":true}`

- [ ] **Step 6: Commit**

```bash
git add server/
git commit -m "feat(server): scaffold Node.js project"
```

---

### Task 2: PostgreSQL schema và DB setup

**Files:**
- Create: `server/src/db/schema.sql`
- Create: `server/src/db/queries.js`

- [ ] **Step 1: Tạo schema.sql**

```sql
-- server/src/db/schema.sql
CREATE TABLE IF NOT EXISTS users (
  id            SERIAL PRIMARY KEY,
  username      VARCHAR(50) UNIQUE NOT NULL,
  password_hash VARCHAR(255) NOT NULL
);

CREATE TABLE IF NOT EXISTS devices (
  id         VARCHAR(32) PRIMARY KEY,
  name       VARCHAR(100) NOT NULL,
  location   VARCHAR(100),
  online     BOOLEAN DEFAULT false,
  last_state JSONB,
  created_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS schedules (
  id         SERIAL PRIMARY KEY,
  device_id  VARCHAR(32) REFERENCES devices(id) ON DELETE CASCADE,
  cron_expr  VARCHAR(50) NOT NULL,
  command    JSONB NOT NULL,
  enabled    BOOLEAN DEFAULT true,
  created_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS sensor_rules (
  id             SERIAL PRIMARY KEY,
  device_id      VARCHAR(32) REFERENCES devices(id) ON DELETE CASCADE,
  sensor_type    VARCHAR(10) NOT NULL,
  condition_op   VARCHAR(5) NOT NULL,
  condition_val  FLOAT NOT NULL,
  command        JSONB NOT NULL,
  enabled        BOOLEAN DEFAULT true,
  created_at     TIMESTAMPTZ DEFAULT NOW()
);
```

- [ ] **Step 2: Chạy schema trên PostgreSQL**

```bash
psql $DATABASE_URL -f src/db/schema.sql
```
Expected: `CREATE TABLE` x4, không có errors.

- [ ] **Step 3: Seed admin user**

```bash
node -e "
const bcrypt = require('bcryptjs');
const { Pool } = require('pg');
const pool = new Pool({ connectionString: process.env.DATABASE_URL });
bcrypt.hash('admin123', 10).then(hash => {
  return pool.query(
    'INSERT INTO users (username, password_hash) VALUES (\$1, \$2) ON CONFLICT DO NOTHING',
    ['admin', hash]
  );
}).then(() => { console.log('Admin seeded'); pool.end(); });
"
```
Expected: `Admin seeded`

- [ ] **Step 4: Tạo src/db/queries.js**

```js
// server/src/db/queries.js
const { Pool } = require('pg');
const pool = new Pool({ connectionString: process.env.DATABASE_URL });

module.exports = {
  // Users
  findUserByUsername: (username) =>
    pool.query('SELECT * FROM users WHERE username = $1', [username])
      .then(r => r.rows[0]),

  // Devices
  getAllDevices: () =>
    pool.query('SELECT * FROM devices ORDER BY created_at DESC')
      .then(r => r.rows),

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

  // Schedules
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

  // Sensor Rules
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
};
```

- [ ] **Step 5: Commit**

```bash
git add server/src/db/
git commit -m "feat(server): add PostgreSQL schema and queries"
```

---

### Task 3: JWT auth middleware và login route

**Files:**
- Create: `server/src/middleware/auth.js`
- Create: `server/src/routes/auth.js`
- Create: `server/tests/auth.test.js`

- [ ] **Step 1: Viết failing test**

```js
// server/tests/auth.test.js
const request = require('supertest');
const app = require('../src/index');

describe('POST /api/auth/login', () => {
  it('returns 400 when missing fields', async () => {
    const res = await request(app).post('/api/auth/login').send({});
    expect(res.status).toBe(400);
  });

  it('returns 401 with wrong password', async () => {
    const res = await request(app)
      .post('/api/auth/login')
      .send({ username: 'admin', password: 'wrong' });
    expect(res.status).toBe(401);
  });

  it('returns token and mqtt config on success', async () => {
    const res = await request(app)
      .post('/api/auth/login')
      .send({ username: 'admin', password: 'admin123' });
    expect(res.status).toBe(200);
    expect(res.body).toHaveProperty('token');
    expect(res.body.mqtt).toHaveProperty('host');
    expect(res.body.mqtt).toHaveProperty('port');
  });
});
```

- [ ] **Step 2: Chạy test — verify fail**

```bash
npx jest tests/auth.test.js --verbose
```
Expected: FAIL — `Cannot POST /api/auth/login`

- [ ] **Step 3: Tạo middleware/auth.js**

```js
// server/src/middleware/auth.js
const jwt = require('jsonwebtoken');

module.exports = (req, res, next) => {
  const header = req.headers.authorization;
  if (!header || !header.startsWith('Bearer '))
    return res.status(401).json({ error: 'Missing token' });
  try {
    req.user = jwt.verify(header.slice(7), process.env.JWT_SECRET);
    next();
  } catch {
    res.status(401).json({ error: 'Invalid token' });
  }
};
```

- [ ] **Step 4: Tạo routes/auth.js**

```js
// server/src/routes/auth.js
const router = require('express').Router();
const bcrypt = require('bcryptjs');
const jwt = require('jsonwebtoken');
const db = require('../db/queries');

router.post('/login', async (req, res) => {
  const { username, password } = req.body;
  if (!username || !password)
    return res.status(400).json({ error: 'username and password required' });

  const user = await db.findUserByUsername(username);
  if (!user) return res.status(401).json({ error: 'Invalid credentials' });

  const ok = await bcrypt.compare(password, user.password_hash);
  if (!ok) return res.status(401).json({ error: 'Invalid credentials' });

  const token = jwt.sign({ id: user.id, username: user.username },
                          process.env.JWT_SECRET,
                          { expiresIn: process.env.JWT_EXPIRES_IN || '7d' });

  res.json({
    token,
    mqtt: {
      host: process.env.MQTT_HOST,
      port: 8883,
      username: process.env.APP_MQTT_USER,
      password: process.env.APP_MQTT_PASS,
    }
  });
});

module.exports = router;
```

- [ ] **Step 5: Register route trong index.js**

```js
// Thêm vào src/index.js sau app.use(express.json()):
const authRoutes = require('./routes/auth');
app.use('/api/auth', authRoutes);
```

- [ ] **Step 6: Chạy test — verify pass**

```bash
npx jest tests/auth.test.js --verbose
```
Expected: PASS (3 tests)

- [ ] **Step 7: Commit**

```bash
git add server/src/middleware/ server/src/routes/auth.js server/tests/auth.test.js
git commit -m "feat(server): add JWT auth middleware and login route"
```

---

### Task 4: Device routes (CRUD)

**Files:**
- Create: `server/src/routes/devices.js`
- Create: `server/tests/devices.test.js`

- [ ] **Step 1: Viết failing test**

```js
// server/tests/devices.test.js
const request = require('supertest');
const app = require('../src/index');
const db = require('../src/db/queries');
let token;

beforeAll(async () => {
  const res = await request(app)
    .post('/api/auth/login')
    .send({ username: 'admin', password: 'admin123' });
  token = res.body.token;
  // clean up test device
  await db.deleteDevice('test_dev_01').catch(() => {});
});

afterAll(async () => {
  await db.deleteDevice('test_dev_01').catch(() => {});
});

describe('GET /api/devices', () => {
  it('returns 401 without token', async () => {
    const res = await request(app).get('/api/devices');
    expect(res.status).toBe(401);
  });
  it('returns array of devices', async () => {
    const res = await request(app)
      .get('/api/devices')
      .set('Authorization', `Bearer ${token}`);
    expect(res.status).toBe(200);
    expect(Array.isArray(res.body)).toBe(true);
  });
});

describe('POST /api/devices', () => {
  it('creates a device', async () => {
    const res = await request(app)
      .post('/api/devices')
      .set('Authorization', `Bearer ${token}`)
      .send({ id: 'test_dev_01', name: 'Test Device', location: 'Room 1' });
    expect(res.status).toBe(201);
    expect(res.body.id).toBe('test_dev_01');
  });
  it('returns 400 when missing id or name', async () => {
    const res = await request(app)
      .post('/api/devices')
      .set('Authorization', `Bearer ${token}`)
      .send({ name: 'No ID' });
    expect(res.status).toBe(400);
  });
});

describe('DELETE /api/devices/:id', () => {
  it('deletes a device', async () => {
    const res = await request(app)
      .delete('/api/devices/test_dev_01')
      .set('Authorization', `Bearer ${token}`);
    expect(res.status).toBe(204);
  });
});
```

- [ ] **Step 2: Chạy test — verify fail**

```bash
npx jest tests/devices.test.js --verbose
```
Expected: FAIL

- [ ] **Step 3: Tạo routes/devices.js**

```js
// server/src/routes/devices.js
const router = require('express').Router();
const db = require('../db/queries');
const authMiddleware = require('../middleware/auth');

router.use(authMiddleware);

router.get('/', async (req, res) => {
  const devices = await db.getAllDevices();
  res.json(devices);
});

router.post('/', async (req, res) => {
  const { id, name, location } = req.body;
  if (!id || !name) return res.status(400).json({ error: 'id and name required' });
  const device = await db.upsertDevice(id, name, location || null);
  res.status(201).json(device);
});

router.delete('/:id', async (req, res) => {
  await db.deleteDevice(req.params.id);
  res.status(204).end();
});

module.exports = router;
```

- [ ] **Step 4: Register route trong index.js**

```js
const deviceRoutes = require('./routes/devices');
app.use('/api/devices', deviceRoutes);
```

- [ ] **Step 5: Chạy test — verify pass**

```bash
npx jest tests/devices.test.js --verbose
```
Expected: PASS (5 tests)

- [ ] **Step 6: Commit**

```bash
git add server/src/routes/devices.js server/tests/devices.test.js
git commit -m "feat(server): add devices CRUD routes"
```

---

### Task 5: rule.service — evaluate sensor rule

**Files:**
- Create: `server/src/services/rule.service.js`
- Create: `server/tests/rule.service.test.js`

- [ ] **Step 1: Viết failing test**

```js
// server/tests/rule.service.test.js
const { evaluateCondition, buildCommandEnvelope } = require('../src/services/rule.service');

describe('evaluateCondition', () => {
  it('== operator matches equal value', () => {
    expect(evaluateCondition(1, '==', 1)).toBe(true);
  });
  it('== operator does not match different value', () => {
    expect(evaluateCondition(0, '==', 1)).toBe(false);
  });
  it('> operator', () => {
    expect(evaluateCondition(300, '>', 200)).toBe(true);
    expect(evaluateCondition(100, '>', 200)).toBe(false);
  });
  it('< operator', () => {
    expect(evaluateCondition(100, '<', 200)).toBe(true);
  });
  it('>= operator', () => {
    expect(evaluateCondition(200, '>=', 200)).toBe(true);
  });
  it('<= operator', () => {
    expect(evaluateCondition(199, '<=', 200)).toBe(true);
  });
});

describe('buildCommandEnvelope', () => {
  it('builds correct envelope structure', () => {
    const env = buildCommandEnvelope('esp_001', { '1': true, '2': 180 });
    expect(env.v).toBe(1);
    expect(env.devId).toBe('esp_001');
    expect(env.type).toBe('cmd');
    expect(env.data.dps['1']).toBe(true);
    expect(typeof env.msgId).toBe('string');
    expect(typeof env.ts).toBe('number');
  });
});
```

- [ ] **Step 2: Chạy test — verify fail**

```bash
npx jest tests/rule.service.test.js --verbose
```
Expected: FAIL

- [ ] **Step 3: Tạo services/rule.service.js**

```js
// server/src/services/rule.service.js
const db = require('../db/queries');
const { randomBytes } = require('crypto');

function evaluateCondition(sensorVal, op, ruleVal) {
  switch (op) {
    case '==': return sensorVal === ruleVal;
    case '!=': return sensorVal !== ruleVal;
    case '>':  return sensorVal > ruleVal;
    case '<':  return sensorVal < ruleVal;
    case '>=': return sensorVal >= ruleVal;
    case '<=': return sensorVal <= ruleVal;
    default:   return false;
  }
}

function buildCommandEnvelope(deviceId, dps) {
  return {
    v: 1,
    msgId: randomBytes(4).toString('hex'),
    ts: Date.now(),
    devId: deviceId,
    type: 'cmd',
    data: { dps }
  };
}

async function processSensorMessage(deviceId, sensorData, mqttPublishFn) {
  const rules = await db.getEnabledRulesByDevice(deviceId);

  for (const rule of rules) {
    const { sensor_type, condition_op, condition_val, command } = rule;

    let sensorVal;
    switch (sensor_type) {
      case 'PIR':      sensorVal = sensorData.pir;  break;
      case 'LDR':      sensorVal = sensorData.ldr;  break;
      case 'DHT_TEMP': sensorVal = sensorData.temp; break;
      case 'DHT_HUM':  sensorVal = sensorData.hum;  break;
      default: continue;
    }

    if (sensorVal === undefined) continue;
    if (!evaluateCondition(sensorVal, condition_op, condition_val)) continue;

    const envelope = buildCommandEnvelope(deviceId, command);
    const topic = `loco/v1/${deviceId}/cmd/led`;
    mqttPublishFn(topic, JSON.stringify(envelope));
  }
}

module.exports = { evaluateCondition, buildCommandEnvelope, processSensorMessage };
```

- [ ] **Step 4: Chạy test — verify pass**

```bash
npx jest tests/rule.service.test.js --verbose
```
Expected: PASS (8 tests)

- [ ] **Step 5: Commit**

```bash
git add server/src/services/rule.service.js server/tests/rule.service.test.js
git commit -m "feat(server): add sensor rule evaluation service"
```

---

### Task 6: scheduler.service — node-cron jobs

**Files:**
- Create: `server/src/services/scheduler.service.js`
- Create: `server/tests/scheduler.service.test.js`

- [ ] **Step 1: Viết failing test**

```js
// server/tests/scheduler.service.test.js
const { createSchedulerService } = require('../src/services/scheduler.service');

describe('createSchedulerService', () => {
  it('loads schedules and creates cron jobs', async () => {
    const published = [];
    const mockDb = {
      getAllEnabledSchedules: async () => [
        { id: 1, device_id: 'esp_001', cron_expr: '* * * * *',
          command: { '1': true, '2': 200 } }
      ]
    };
    const mockPublish = (topic, msg) => published.push({ topic, msg });

    const scheduler = createSchedulerService(mockDb, mockPublish);
    await scheduler.loadAll();

    expect(scheduler.jobCount()).toBe(1);
    scheduler.stopAll();
  });

  it('reloads when schedule is added', async () => {
    let schedules = [];
    const mockDb = { getAllEnabledSchedules: async () => schedules };
    const mockPublish = () => {};

    const scheduler = createSchedulerService(mockDb, mockPublish);
    await scheduler.loadAll();
    expect(scheduler.jobCount()).toBe(0);

    schedules = [{ id: 2, device_id: 'esp_002', cron_expr: '0 8 * * *',
                   command: { '1': false } }];
    await scheduler.reload();
    expect(scheduler.jobCount()).toBe(1);
    scheduler.stopAll();
  });
});
```

- [ ] **Step 2: Chạy test — verify fail**

```bash
npx jest tests/scheduler.service.test.js --verbose
```
Expected: FAIL

- [ ] **Step 3: Tạo services/scheduler.service.js**

```js
// server/src/services/scheduler.service.js
const cron = require('node-cron');
const { buildCommandEnvelope } = require('./rule.service');

function createSchedulerService(db, mqttPublishFn) {
  const jobs = new Map(); // scheduleId → cron.ScheduledTask

  function stopAll() {
    for (const job of jobs.values()) job.stop();
    jobs.clear();
  }

  function startJob(schedule) {
    if (!cron.validate(schedule.cron_expr)) {
      console.warn(`Invalid cron expr for schedule ${schedule.id}: ${schedule.cron_expr}`);
      return;
    }
    const job = cron.schedule(schedule.cron_expr, () => {
      const envelope = buildCommandEnvelope(schedule.device_id, schedule.command);
      const topic = `loco/v1/${schedule.device_id}/cmd/led`;
      mqttPublishFn(topic, JSON.stringify(envelope));
    });
    jobs.set(schedule.id, job);
  }

  async function loadAll() {
    stopAll();
    const schedules = await db.getAllEnabledSchedules();
    for (const s of schedules) startJob(s);
    console.log(`Scheduler: loaded ${jobs.size} jobs`);
  }

  async function reload() {
    await loadAll();
  }

  return { loadAll, reload, stopAll, jobCount: () => jobs.size };
}

module.exports = { createSchedulerService };
```

- [ ] **Step 4: Chạy test — verify pass**

```bash
npx jest tests/scheduler.service.test.js --verbose
```
Expected: PASS (2 tests)

- [ ] **Step 5: Commit**

```bash
git add server/src/services/scheduler.service.js server/tests/scheduler.service.test.js
git commit -m "feat(server): add node-cron scheduler service"
```

---

### Task 7: mqtt.service — MQTT client + message routing

**Files:**
- Create: `server/src/services/mqtt.service.js`

- [ ] **Step 1: Tạo services/mqtt.service.js**

```js
// server/src/services/mqtt.service.js
const mqtt = require('mqtt');
const db = require('../db/queries');
const { processSensorMessage } = require('./rule.service');

let client;

function publish(topic, message) {
  if (client && client.connected) {
    client.publish(topic, message);
  }
}

function connect() {
  const options = {
    host: process.env.MQTT_HOST,
    port: parseInt(process.env.MQTT_PORT) || 8883,
    username: process.env.MQTT_USER,
    password: process.env.MQTT_PASS,
    rejectUnauthorized: false, // TLS without cert validation
    keepalive: 120,
    clientId: 'loco-server-' + Date.now(),
  };

  const protocol = process.env.MQTT_USE_TLS === 'true' ? 'mqtts' : 'mqtt';
  client = mqtt.connect(`${protocol}://${options.host}`, options);

  client.on('connect', () => {
    console.log('MQTT connected');
    client.subscribe('loco/v1/+/rpt/state');
    client.subscribe('loco/v1/+/rpt/sensor');
    client.subscribe('loco/v1/+/evt/status');
  });

  client.on('message', async (topic, payload) => {
    let msg;
    try { msg = JSON.parse(payload.toString()); } catch { return; }

    const parts = topic.split('/'); // loco/v1/{devId}/rpt|evt/{subtype}
    if (parts.length < 5) return;
    const deviceId = parts[2];
    const msgType  = parts[3]; // rpt / evt
    const subtype  = parts[4]; // state / sensor / status

    if (msgType === 'evt' && subtype === 'status') {
      const online = msg?.data?.online === true;
      await db.updateDeviceOnline(deviceId, online).catch(() => {});
    }

    if (msgType === 'rpt' && subtype === 'state' && msg?.data?.dps) {
      await db.updateDeviceState(deviceId, msg.data.dps).catch(() => {});
    }

    if (msgType === 'rpt' && subtype === 'sensor' && msg?.data?.sensors) {
      await processSensorMessage(deviceId, msg.data.sensors, publish).catch(console.error);
    }
  });

  client.on('error', (err) => console.error('MQTT error:', err.message));
  client.on('reconnect', () => console.log('MQTT reconnecting...'));
}

module.exports = { connect, publish };
```

- [ ] **Step 2: Tích hợp vào index.js**

```js
// Thêm vào src/index.js trước app.listen():
const mqttService = require('./services/mqtt.service');
const { createSchedulerService } = require('./services/scheduler.service');
const db = require('./db/queries');

mqttService.connect();
const scheduler = createSchedulerService(db, mqttService.publish);
scheduler.loadAll();
```

- [ ] **Step 3: Verify MQTT connects**

```bash
node src/index.js
```
Expected log: `MQTT connected`

- [ ] **Step 4: Commit**

```bash
git add server/src/services/mqtt.service.js server/src/index.js
git commit -m "feat(server): add MQTT service with sensor/state/status routing"
```

---

### Task 8: Schedule và Rule routes

**Files:**
- Create: `server/src/routes/schedules.js`
- Create: `server/src/routes/rules.js`

- [ ] **Step 1: Tạo routes/schedules.js**

```js
// server/src/routes/schedules.js
const router = require('express').Router();
const cron = require('node-cron');
const db = require('../db/queries');
const authMiddleware = require('../middleware/auth');

let _scheduler; // injected sau khi scheduler init

router.use(authMiddleware);

router.get('/', async (req, res) => {
  const { device_id } = req.query;
  if (!device_id) return res.status(400).json({ error: 'device_id required' });
  const rows = await db.getSchedulesByDevice(device_id);
  res.json(rows);
});

router.post('/', async (req, res) => {
  const { device_id, cron_expr, command } = req.body;
  if (!device_id || !cron_expr || !command)
    return res.status(400).json({ error: 'device_id, cron_expr, command required' });
  if (!cron.validate(cron_expr))
    return res.status(400).json({ error: 'invalid cron_expr' });
  const row = await db.createSchedule(device_id, cron_expr, command);
  if (_scheduler) await _scheduler.reload();
  res.status(201).json(row);
});

router.put('/:id', async (req, res) => {
  const { cron_expr, command, enabled } = req.body;
  if (cron_expr && !cron.validate(cron_expr))
    return res.status(400).json({ error: 'invalid cron_expr' });
  const row = await db.updateSchedule(req.params.id, cron_expr, command, enabled);
  if (_scheduler) await _scheduler.reload();
  res.json(row);
});

router.delete('/:id', async (req, res) => {
  await db.deleteSchedule(req.params.id);
  if (_scheduler) await _scheduler.reload();
  res.status(204).end();
});

router.setScheduler = (s) => { _scheduler = s; };

module.exports = router;
```

- [ ] **Step 2: Tạo routes/rules.js**

```js
// server/src/routes/rules.js
const router = require('express').Router();
const db = require('../db/queries');
const authMiddleware = require('../middleware/auth');

router.use(authMiddleware);

router.get('/', async (req, res) => {
  const { device_id } = req.query;
  if (!device_id) return res.status(400).json({ error: 'device_id required' });
  const rows = await db.getRulesByDevice(device_id);
  res.json(rows);
});

router.post('/', async (req, res) => {
  const { device_id, sensor_type, condition_op, condition_val, command } = req.body;
  if (!device_id || !sensor_type || !condition_op || condition_val === undefined || !command)
    return res.status(400).json({ error: 'all fields required' });
  const VALID_SENSORS = ['PIR', 'LDR', 'DHT_TEMP', 'DHT_HUM'];
  const VALID_OPS     = ['==', '!=', '>', '<', '>=', '<='];
  if (!VALID_SENSORS.includes(sensor_type))
    return res.status(400).json({ error: 'invalid sensor_type' });
  if (!VALID_OPS.includes(condition_op))
    return res.status(400).json({ error: 'invalid condition_op' });
  const row = await db.createRule(device_id, sensor_type, condition_op, condition_val, command);
  res.status(201).json(row);
});

router.put('/:id', async (req, res) => {
  const { sensor_type, condition_op, condition_val, command, enabled } = req.body;
  const row = await db.updateRule(req.params.id, sensor_type, condition_op,
                                  condition_val, command, enabled);
  res.json(row);
});

router.delete('/:id', async (req, res) => {
  await db.deleteRule(req.params.id);
  res.status(204).end();
});

module.exports = router;
```

- [ ] **Step 3: Register routes trong index.js**

```js
const scheduleRoutes = require('./routes/schedules');
const ruleRoutes     = require('./routes/rules');
app.use('/api/schedules', scheduleRoutes);
app.use('/api/rules',     ruleRoutes);

// Inject scheduler vào scheduleRoutes sau khi scheduler được tạo:
scheduleRoutes.setScheduler(scheduler);
```

- [ ] **Step 4: Verify toàn bộ test suite**

```bash
npx jest --verbose
```
Expected: tất cả PASS

- [ ] **Step 5: Commit**

```bash
git add server/src/routes/schedules.js server/src/routes/rules.js server/src/index.js
git commit -m "feat(server): add schedule and sensor rule CRUD routes"
```

---

### Task 9: Mosquitto setup trên VPS

**Files:**
- Create: `/etc/mosquitto/conf.d/loco.conf` (trên VPS)
- Create: `/etc/mosquitto/passwd` (trên VPS)

- [ ] **Step 1: Cài Mosquitto trên VPS**

```bash
sudo apt update && sudo apt install -y mosquitto mosquitto-clients
sudo systemctl enable mosquitto
```

- [ ] **Step 2: Tạo password file**

```bash
# Tạo user cho server
sudo mosquitto_passwd -c /etc/mosquitto/passwd server
# Thêm user cho app
sudo mosquitto_passwd /etc/mosquitto/passwd app_user
# Thêm user cho mỗi device (script tự động hoá sau)
sudo mosquitto_passwd /etc/mosquitto/passwd esp_001
```

- [ ] **Step 3: Tạo /etc/mosquitto/conf.d/loco.conf**

```conf
# /etc/mosquitto/conf.d/loco.conf
listener 8883
cafile /etc/mosquitto/certs/ca.crt
certfile /etc/mosquitto/certs/server.crt
keyfile /etc/mosquitto/certs/server.key

allow_anonymous false
password_file /etc/mosquitto/passwd

max_connections 600
max_queued_messages 1000
```

- [ ] **Step 4: Tạo self-signed TLS certificate**

```bash
mkdir -p /etc/mosquitto/certs && cd /etc/mosquitto/certs
openssl req -new -x509 -days 3650 -extensions v3_ca \
  -keyout ca.key -out ca.crt -subj "/CN=loco-ca"
openssl genrsa -out server.key 2048
openssl req -new -key server.key -out server.csr -subj "/CN=your-vps.com"
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key \
  -CAcreateserial -out server.crt -days 3650
chmod 640 server.key && sudo chown mosquitto:mosquitto server.key
```

- [ ] **Step 5: Restart và verify**

```bash
sudo systemctl restart mosquitto
mosquitto_sub -h localhost -p 8883 --insecure -u server -P password -t test &
mosquitto_pub -h localhost -p 8883 --insecure -u server -P password -t test -m "ok"
```
Expected: nhận message "ok"

- [ ] **Step 6: Commit notes**

```bash
git add server/
git commit -m "docs(server): add Mosquitto setup instructions in plan"
```

---

### Task 10: Chạy full system test

- [ ] **Step 1: Start server**

```bash
cd server && node src/index.js
```
Expected: `Server running on port 3000` + `MQTT connected` + `Scheduler: loaded N jobs`

- [ ] **Step 2: Test login**

```bash
curl -X POST http://localhost:3000/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}'
```
Expected: `{"token":"...","mqtt":{"host":"...","port":8883}}`

- [ ] **Step 3: Đăng ký thiết bị**

```bash
TOKEN="<token từ bước trên>"
curl -X POST http://localhost:3000/api/devices \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"id":"esp_001","name":"LED phòng khách","location":"Phòng khách"}'
```
Expected: `{"id":"esp_001",...}`

- [ ] **Step 4: Tạo sensor rule**

```bash
curl -X POST http://localhost:3000/api/rules \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"device_id":"esp_001","sensor_type":"PIR","condition_op":"==","condition_val":1,"command":{"1":true,"2":200,"3":1,"4":"255,165,0"}}'
```
Expected: `{"id":1,...}`

- [ ] **Step 5: Simulate sensor trigger**

```bash
# Publish sensor data như thể là ESP8266
mosquitto_pub -h localhost -p 8883 --insecure -u server -P password \
  -t "loco/v1/esp_001/rpt/sensor" \
  -m '{"v":1,"msgId":"test","ts":0,"devId":"esp_001","type":"rpt","data":{"sensors":{"pir":1}}}'

# Subscribe để thấy LED command được publish
mosquitto_sub -h localhost -p 8883 --insecure -u server -P password \
  -t "loco/v1/esp_001/cmd/led"
```
Expected: server publish LED command với DPs từ rule

- [ ] **Step 6: Commit**

```bash
git add server/
git commit -m "feat(server): server backend complete - all routes and services wired"
```
