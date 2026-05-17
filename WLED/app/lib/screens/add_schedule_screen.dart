// app/lib/screens/add_schedule_screen.dart
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../models/device.dart';
import '../providers/auth_provider.dart';

class AddScheduleScreen extends ConsumerStatefulWidget {
  final String deviceId;
  const AddScheduleScreen({super.key, required this.deviceId});
  @override
  ConsumerState<AddScheduleScreen> createState() => _AddScheduleScreenState();
}

class _AddScheduleScreenState extends ConsumerState<AddScheduleScreen> {
  final _cronCtrl = TextEditingController(text: '0 8 * * *');
  LedState _cmd = const LedState(on: true, brightness: 200, effect: 1);
  bool _loading = false;

  Future<void> _save() async {
    setState(() => _loading = true);
    try {
      await ref.read(apiServiceProvider).createSchedule(
        widget.deviceId, _cronCtrl.text.trim(), _cmd.toDps());
      if (mounted) Navigator.pop(context);
    } catch (e) {
      if (mounted) ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text('$e')));
    } finally {
      setState(() => _loading = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Thêm lịch hẹn')),
      body: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(children: [
          TextField(controller: _cronCtrl,
              decoration: const InputDecoration(
                labelText: 'Cron expression',
                helperText: 'VD: 0 8 * * 1-5 (8 giờ sáng, thứ 2-6)')),
          const SizedBox(height: 12),
          const Text('Lệnh LED:', style: TextStyle(fontWeight: FontWeight.bold)),
          SwitchListTile(title: const Text('Bật đèn'),
              value: _cmd.on,
              onChanged: (v) => setState(() => _cmd = _cmd.copyWith(on: v))),
          ListTile(
            title: Text('Độ sáng: ${_cmd.brightness}'),
            subtitle: Slider(
              value: _cmd.brightness.toDouble(), min: 0, max: 255,
              onChanged: (v) => setState(() => _cmd = _cmd.copyWith(brightness: v.toInt())),
            ),
          ),
          const SizedBox(height: 24),
          ElevatedButton(onPressed: _loading ? null : _save,
              child: _loading ? const CircularProgressIndicator() : const Text('Lưu lịch')),
        ]),
      ),
    );
  }
}
