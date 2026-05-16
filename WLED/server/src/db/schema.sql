CREATE TABLE IF NOT EXISTS users (
  id            SERIAL PRIMARY KEY,
  username      VARCHAR(50) UNIQUE NOT NULL,
  password_hash VARCHAR(255) NOT NULL
);

CREATE TABLE IF NOT EXISTS devices (
  id         VARCHAR(32) PRIMARY KEY,
  name       VARCHAR(100) NOT NULL,
  location   VARCHAR(100),
  online     BOOLEAN DEFAULT false,
  last_state JSONB,
  created_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS schedules (
  id         SERIAL PRIMARY KEY,
  device_id  VARCHAR(32) REFERENCES devices(id) ON DELETE CASCADE,
  cron_expr  VARCHAR(50) NOT NULL,
  command    JSONB NOT NULL,
  enabled    BOOLEAN DEFAULT true,
  created_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS sensor_rules (
  id             SERIAL PRIMARY KEY,
  device_id      VARCHAR(32) REFERENCES devices(id) ON DELETE CASCADE,
  sensor_type    VARCHAR(20) NOT NULL CHECK (sensor_type IN ('temperature', 'humidity', 'motion', 'light')),
  condition_op   VARCHAR(5) NOT NULL CHECK (condition_op IN ('>', '<', '>=', '<=', '=')),
  condition_val  FLOAT NOT NULL,
  command        JSONB NOT NULL,
  enabled        BOOLEAN DEFAULT true,
  created_at     TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_schedules_device_id ON schedules(device_id);
CREATE INDEX IF NOT EXISTS idx_sensor_rules_device_id ON sensor_rules(device_id);
CREATE INDEX IF NOT EXISTS idx_schedules_enabled ON schedules(enabled) WHERE enabled = true;
