const cron = require('node-cron');
const { buildCommandEnvelope } = require('./rule.service');

function createSchedulerService(db, mqttPublishFn) {
  const jobs = new Map();

  function stopAll() {
    for (const job of jobs.values()) job.stop();
    jobs.clear();
  }

  function startJob(schedule) {
    if (!cron.validate(schedule.cron_expr)) {
      console.warn(`Invalid cron expr for schedule ${schedule.id}: ${schedule.cron_expr}`);
      return;
    }
    const job = cron.schedule(schedule.cron_expr, () => {
      const envelope = buildCommandEnvelope(schedule.device_id, schedule.command);
      const topic = `loco/v1/${schedule.device_id}/cmd/led`;
      mqttPublishFn(topic, JSON.stringify(envelope));
    });
    jobs.set(schedule.id, job);
  }

  async function loadAll() {
    stopAll();
    const schedules = await db.getAllEnabledSchedules();
    for (const s of schedules) startJob(s);
    console.log(`Scheduler: loaded ${jobs.size} jobs`);
  }

  async function reload() {
    await loadAll();
  }

  return { loadAll, reload, stopAll, jobCount: () => jobs.size };
}

module.exports = { createSchedulerService };
