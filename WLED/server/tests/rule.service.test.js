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
