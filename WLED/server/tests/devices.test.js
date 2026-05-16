const request = require('supertest');
const app = require('../src/index');
const db = require('../src/db/queries');
let token;

beforeAll(async () => {
  const res = await request(app)
    .post('/api/auth/login')
    .send({ username: 'admin', password: 'admin123' });
  token = res.body.token;
  await db.deleteDevice('test_dev_01').catch(() => {});
});

afterAll(async () => {
  await db.deleteDevice('test_dev_01').catch(() => {});
  await db.end();
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
