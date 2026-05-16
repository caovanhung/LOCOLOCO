const request = require('supertest');
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
    const res = await request(app)
      .post('/api/auth/login')
      .send({ username: 'admin', password: 'admin123' });
    expect(res.status).toBe(200);
    expect(res.body).toHaveProperty('token');
    expect(res.body.mqtt).toHaveProperty('host');
    expect(res.body.mqtt).toHaveProperty('port');
  });
});
