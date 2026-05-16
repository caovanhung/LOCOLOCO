const { createSchedulerService } = require('../src/services/scheduler.service');

describe('createSchedulerService', () => {
  it('loads schedules and creates cron jobs', async () => {
    const published = [];
    const mockDb = {
      getAllEnabledSchedules: async () => [
        { id: 1, device_id: 'esp_001', cron_expr: '* * * * *',
          command: { '1': true, '2': 200 } }
      ]
    };
    const mockPublish = (topic, msg) => published.push({ topic, msg });

    const scheduler = createSchedulerService(mockDb, mockPublish);
    await scheduler.loadAll();

    expect(scheduler.jobCount()).toBe(1);
    scheduler.stopAll();
  });

  it('reloads when schedule is added', async () => {
    let schedules = [];
    const mockDb = { getAllEnabledSchedules: async () => schedules };
    const mockPublish = () => {};

    const scheduler = createSchedulerService(mockDb, mockPublish);
    await scheduler.loadAll();
    expect(scheduler.jobCount()).toBe(0);

    schedules = [{ id: 2, device_id: 'esp_002', cron_expr: '0 8 * * *',
                   command: { '1': false } }];
    await scheduler.reload();
    expect(scheduler.jobCount()).toBe(1);
    scheduler.stopAll();
  });
});
