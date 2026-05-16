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
      case 'PIR':      sensorVal = sensorData.pir ? 1 : 0;  break;
      case 'LDR':      sensorVal = sensorData.ldr;           break;
      case 'DHT_TEMP': sensorVal = sensorData.temp;          break;
      case 'DHT_HUM':  sensorVal = sensorData.hum;           break;
      default: continue;
    }

    if (sensorVal === undefined || sensorVal === null) continue;
    if (!evaluateCondition(sensorVal, condition_op, Number(condition_val))) continue;

    const envelope = buildCommandEnvelope(deviceId, command);
    const topic = `loco/v1/${deviceId}/cmd/led`;
    mqttPublishFn(topic, JSON.stringify(envelope));
  }
}

module.exports = { evaluateCondition, buildCommandEnvelope, processSensorMessage };
