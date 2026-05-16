const router = require('express').Router();
const db = require('../db/queries');
const authMiddleware = require('../middleware/auth');

router.use(authMiddleware);

router.get('/', async (req, res) => {
  try {
    const devices = await db.getAllDevices();
    res.json(devices);
  } catch (err) {
    console.error('GET /devices error', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

router.post('/', async (req, res) => {
  const { id, name, location } = req.body;
  if (!id || !name) return res.status(400).json({ error: 'id and name required' });
  try {
    const device = await db.upsertDevice(id, name, location || null);
    res.status(201).json(device);
  } catch (err) {
    console.error('POST /devices error', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

router.delete('/:id', async (req, res) => {
  try {
    await db.deleteDevice(req.params.id);
    res.status(204).end();
  } catch (err) {
    console.error('DELETE /devices error', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

module.exports = router;
