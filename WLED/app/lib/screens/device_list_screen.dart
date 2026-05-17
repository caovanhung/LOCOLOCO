// app/lib/screens/device_list_screen.dart
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../providers/device_provider.dart';
import '../models/device.dart';
import 'device_detail_screen.dart';
import 'add_device_screen.dart';

class DeviceListScreen extends ConsumerWidget {
  const DeviceListScreen({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final devicesAsync = ref.watch(deviceListProvider);
    return Scaffold(
      appBar: AppBar(title: const Text('Thiết bị')),
      floatingActionButton: FloatingActionButton(
        onPressed: () async {
          await Navigator.push(context, MaterialPageRoute(builder: (_) => const AddDeviceScreen()));
          ref.invalidate(deviceListProvider);
        },
        child: const Icon(Icons.add),
      ),
      body: devicesAsync.when(
        loading: () => const Center(child: CircularProgressIndicator()),
        error: (e, _) => Center(child: Text('Lỗi: $e')),
        data: (devices) => devices.isEmpty
            ? const Center(child: Text('Chưa có thiết bị nào'))
            : ListView.builder(
                itemCount: devices.length,
                itemBuilder: (ctx, i) {
                  final d = devices[i];
                  return ListTile(
                    leading: Icon(Icons.lightbulb,
                        color: d.online ? Colors.amber : Colors.grey),
                    title: Text(d.name),
                    subtitle: Text(d.location ?? d.id),
                    trailing: const Icon(Icons.chevron_right),
                    onTap: () => Navigator.push(context,
                        MaterialPageRoute(builder: (_) => DeviceDetailScreen(device: d))),
                  );
                },
              ),
      ),
    );
  }
}
