// app/lib/screens/device_list_screen.dart
// Option A: devices auto-appear after ESP8266 connects — no manual add needed.
// User just pulls-to-refresh or taps refresh icon.
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../providers/device_provider.dart';
import 'device_detail_screen.dart';

class DeviceListScreen extends ConsumerWidget {
  const DeviceListScreen({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final devicesAsync = ref.watch(deviceListProvider);
    return Scaffold(
      appBar: AppBar(
        title: const Text('Thiết bị'),
        actions: [
          IconButton(
            icon: const Icon(Icons.refresh),
            tooltip: 'Làm mới danh sách',
            onPressed: () => ref.invalidate(deviceListProvider),
          ),
        ],
      ),
      body: devicesAsync.when(
        loading: () => const Center(child: CircularProgressIndicator()),
        error: (e, _) => Center(child: Text('Lỗi: $e')),
        data: (devices) => RefreshIndicator(
          onRefresh: () async => ref.invalidate(deviceListProvider),
          child: devices.isEmpty
              ? const SingleChildScrollView(
                  physics: AlwaysScrollableScrollPhysics(),
                  child: SizedBox(
                    height: 400,
                    child: Center(
                      child: Column(
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          Icon(Icons.power_off, size: 64, color: Colors.grey),
                          SizedBox(height: 16),
                          Text('Chưa có thiết bị nào',
                              style: TextStyle(color: Colors.grey)),
                          SizedBox(height: 8),
                          Text('Cắm điện ESP8266 để thiết bị tự đăng ký',
                              style: TextStyle(color: Colors.grey, fontSize: 12)),
                        ],
                      ),
                    ),
                  ),
                )
              : ListView.builder(
                  physics: const AlwaysScrollableScrollPhysics(),
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
      ),
    );
  }
}
