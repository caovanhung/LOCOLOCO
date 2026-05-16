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
    rejectUnauthorized: false,
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

    const parts = topic.split('/');
    if (parts.length < 5) return;
    const deviceId = parts[2];
    const msgType  = parts[3];
    const subtype  = parts[4];

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
