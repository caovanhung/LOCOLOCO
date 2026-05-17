// app/lib/screens/add_device_screen.dart
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../providers/auth_provider.dart';

class AddDeviceScreen extends ConsumerStatefulWidget {
  const AddDeviceScreen({super.key});
  @override
  ConsumerState<AddDeviceScreen> createState() => _AddDeviceScreenState();
}

class _AddDeviceScreenState extends ConsumerState<AddDeviceScreen> {
  final _idCtrl   = TextEditingController();
  final _nameCtrl = TextEditingController();
  final _locCtrl  = TextEditingController();
  bool _loading = false;

  Future<void> _save() async {
    if (_idCtrl.text.isEmpty || _nameCtrl.text.isEmpty) return;
    setState(() => _loading = true);
    try {
      final api = ref.read(apiServiceProvider);
      await api.addDevice(_idCtrl.text.trim(), _nameCtrl.text.trim(),
                          _locCtrl.text.isEmpty ? null : _locCtrl.text.trim());
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
      appBar: AppBar(title: const Text('Thêm thiết bị')),
      body: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(children: [
          TextField(controller: _idCtrl,   decoration: const InputDecoration(labelText: 'Device ID (vd: esp_001)')),
          TextField(controller: _nameCtrl, decoration: const InputDecoration(labelText: 'Tên thiết bị')),
          TextField(controller: _locCtrl,  decoration: const InputDecoration(labelText: 'Vị trí (tùy chọn)')),
          const SizedBox(height: 24),
          ElevatedButton(
            onPressed: _loading ? null : _save,
            child: _loading ? const CircularProgressIndicator() : const Text('Lưu'),
          ),
        ]),
      ),
    );
  }
}
