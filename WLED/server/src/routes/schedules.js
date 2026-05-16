const router = require('express').Router();
const cron = require('node-cron');
const db = require('../db/queries');
const authMiddleware = require('../middleware/auth');

let _scheduler;

router.use(authMiddleware);

router.get('/', async (req, res) => {
  const { device_id } = req.query;
  if (!device_id) return res.status(400).json({ error: 'device_id required' });
  try {
    const rows = await db.getSchedulesByDevice(device_id);
    res.json(rows);
  } catch (err) {
    console.error('GET /schedules error', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

router.post('/', async (req, res) => {
  const { device_id, cron_expr, command } = req.body;
  if (!device_id || !cron_expr || !command)
    return res.status(400).json({ error: 'device_id, cron_expr, command required' });
  if (!cron.validate(cron_expr))
    return res.status(400).json({ error: 'invalid cron_expr' });
  try {
    const row = await db.createSchedule(device_id, cron_expr, command);
    if (_scheduler) await _scheduler.reload();
    res.status(201).json(row);
  } catch (err) {
    console.error('POST /schedules error', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

router.put('/:id', async (req, res) => {
  const { cron_expr, command, enabled } = req.body;
  if (cron_expr && !cron.validate(cron_expr))
    return res.status(400).json({ error: 'invalid cron_expr' });
  try {
    const row = await db.updateSchedule(req.params.id, cron_expr, command, enabled);
    if (_scheduler) await _scheduler.reload();
    res.json(row);
  } catch (err) {
    console.error('PUT /schedules error', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

router.delete('/:id', async (req, res) => {
  try {
    await db.deleteSchedule(req.params.id);
    if (_scheduler) await _scheduler.reload();
    res.status(204).end();
  } catch (err) {
    console.error('DELETE /schedules error', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

router.setScheduler = (s) => { _scheduler = s; };

module.exports = router;
