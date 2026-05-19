const request = require('supertest');

// Set required environment variables for tests
process.env.JWT_SECRET = 'test-secret-key';
process.env.JWT_EXPIRES_IN = '7d';

// Mock the database before importing app
jest.mock('../src/db/queries', () => ({
  findUserByUsername: jest.fn((username) => {
    if (username === 'admin') {
      return Promise.resolve({
        id: 1,
        username: 'admin',
        password_hash: '$2b$10$YrhaoKil2JSIymr13jNda.HEpnVWVSmhURrUQVHZAS1/umOBJqZka' // hash of 'admin123'
      });
    }
    return Promise.resolve(null);
  }),
  end: jest.fn(() => Promise.resolve()),
}));

const app = require('../src/index');
const db = require('../src/db/queries');

describe('POST /api/auth/login', () => {
  afterAll(async () => {
    await db.end();
  });

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
    process.env.APP_MQTT_HOST = 'mqtt.example.com';
    process.env.APP_MQTT_PORT = '8883';
    const res = await request(app)
      .post('/api/auth/login')
      .send({ username: 'admin', password: 'admin123' });
    delete process.env.APP_MQTT_HOST;
    delete process.env.APP_MQTT_PORT;
    expect(res.status).toBe(200);
    expect(res.body).toHaveProperty('token');
    expect(res.body.mqtt).toHaveProperty('host');
    expect(res.body.mqtt).toHaveProperty('port');
  });

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
});
