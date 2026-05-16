require('dotenv').config();
const express = require('express');
const app = express();
app.use(express.json());
const authRoutes = require('./routes/auth');
const deviceRoutes = require('./routes/devices');
app.use('/api/auth', authRoutes);
app.use('/api/devices', deviceRoutes);
app.get('/health', (_, res) => res.json({ ok: true }));
if (require.main === module) {
  if (!process.env.JWT_SECRET) throw new Error('JWT_SECRET env var is required');
  const PORT = process.env.PORT || 3000;
  const server = app.listen(PORT, () => console.log(`Server running on port ${PORT}`));
  process.on('SIGTERM', () => server.close(() => process.exit(0)));
  process.on('SIGINT',  () => server.close(() => process.exit(0)));

  const mqttService = require('./services/mqtt.service');
  const { createSchedulerService } = require('./services/scheduler.service');
  const db = require('./db/queries');
  mqttService.connect();
  const scheduler = createSchedulerService(db, mqttService.publish);
  scheduler.loadAll();
}
module.exports = app;
