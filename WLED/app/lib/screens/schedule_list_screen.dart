// app/lib/screens/schedule_list_screen.dart
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../providers/auth_provider.dart';
import '../models/schedule.dart';
import 'add_schedule_screen.dart';

final _schedulesProvider = FutureProvider.family<List<Schedule>, String>((ref, deviceId) async {
  return ref.watch(apiServiceProvider).getSchedules(deviceId);
});

class ScheduleListScreen extends ConsumerWidget {
  final String deviceId;
  const ScheduleListScreen({super.key, required this.deviceId});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final asyncSchedules = ref.watch(_schedulesProvider(deviceId));
    return Scaffold(
      floatingActionButton: FloatingActionButton(
        mini: true,
        onPressed: () async {
          await Navigator.push(context,
              MaterialPageRoute(builder: (_) => AddScheduleScreen(deviceId: deviceId)));
          ref.invalidate(_schedulesProvider(deviceId));
        },
        child: const Icon(Icons.add),
      ),
      body: asyncSchedules.when(
        loading: () => const Center(child: CircularProgressIndicator()),
        error: (e, _) => Center(child: Text('Lỗi: $e')),
        data: (schedules) => schedules.isEmpty
            ? const Center(child: Text('Chưa có lịch hẹn'))
            : ListView.builder(
                itemCount: schedules.length,
                itemBuilder: (ctx, i) {
                  final s = schedules[i];
                  return ListTile(
                    leading: const Icon(Icons.schedule),
                    title: Text(s.cronExpr),
                    subtitle: Text('Effect: ${s.command['3'] ?? 0}  Bri: ${s.command['2'] ?? 200}'),
                    trailing: IconButton(
                      icon: const Icon(Icons.delete, color: Colors.red),
                      onPressed: () async {
                        await ref.read(apiServiceProvider).deleteSchedule(s.id);
                        ref.invalidate(_schedulesProvider(deviceId));
                      },
                    ),
                  );
                },
              ),
      ),
    );
  }
}
