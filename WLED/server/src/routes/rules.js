const router = require('express').Router();
const db = require('../db/queries');
const authMiddleware = require('../middleware/auth');

const VALID_SENSORS = ['PIR', 'LDR', 'DHT_TEMP', 'DHT_HUM'];
const VALID_OPS     = ['==', '!=', '>', '<', '>=', '<='];

router.use(authMiddleware);

router.get('/', async (req, res) => {
  const { device_id } = req.query;
  if (!device_id) return res.status(400).json({ error: 'device_id required' });
  try {
    const rows = await db.getRulesByDevice(device_id);
    res.json(rows);
  } catch (err) {
    console.error('GET /rules error', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

router.post('/', async (req, res) => {
  const { device_id, sensor_type, condition_op, condition_val, command } = req.body;
  if (!device_id || !sensor_type || !condition_op || condition_val === undefined || !command)
    return res.status(400).json({ error: 'all fields required' });
  if (!VALID_SENSORS.includes(sensor_type))
    return res.status(400).json({ error: 'invalid sensor_type' });
  if (!VALID_OPS.includes(condition_op))
    return res.status(400).json({ error: 'invalid condition_op' });
  try {
    const row = await db.createRule(device_id, sensor_type, condition_op, condition_val, command);
    res.status(201).json(row);
  } catch (err) {
    console.error('POST /rules error', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

router.put('/:id', async (req, res) => {
  const { sensor_type, condition_op, condition_val, command, enabled } = req.body;
  try {
    const row = await db.updateRule(req.params.id, sensor_type, condition_op,
                                    condition_val, command, enabled);
    res.json(row);
  } catch (err) {
    console.error('PUT /rules error', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

router.delete('/:id', async (req, res) => {
  try {
    await db.deleteRule(req.params.id);
    res.status(204).end();
  } catch (err) {
    console.error('DELETE /rules error', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

module.exports = router;
