// app/lib/screens/add_rule_screen.dart
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../models/device.dart';
import '../providers/auth_provider.dart';

class AddRuleScreen extends ConsumerStatefulWidget {
  final String deviceId;
  const AddRuleScreen({super.key, required this.deviceId});
  @override
  ConsumerState<AddRuleScreen> createState() => _AddRuleScreenState();
}

class _AddRuleScreenState extends ConsumerState<AddRuleScreen> {
  String _sensor = 'PIR';
  String _op = '==';
  final _valCtrl = TextEditingController(text: '1');
  LedState _cmd = const LedState(on: true, brightness: 200, effect: 1);
  bool _loading = false;

  final _sensors = ['PIR', 'LDR', 'DHT_TEMP', 'DHT_HUM'];
  final _ops     = ['==', '!=', '>', '<', '>=', '<='];

  Future<void> _save() async {
    final val = double.tryParse(_valCtrl.text);
    if (val == null) return;
    setState(() => _loading = true);
    try {
      await ref.read(apiServiceProvider).createRule(
          widget.deviceId, _sensor, _op, val, _cmd.toDps());
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
      appBar: AppBar(title: const Text('Thêm sensor rule')),
      body: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(children: [
          Row(children: [
            const Text('Sensor: '),
            DropdownButton<String>(value: _sensor, items: _sensors.map((s) =>
                DropdownMenuItem(value: s, child: Text(s))).toList(),
                onChanged: (v) { if (v != null) setState(() => _sensor = v); }),
            const SizedBox(width: 12),
            DropdownButton<String>(value: _op, items: _ops.map((o) =>
                DropdownMenuItem(value: o, child: Text(o))).toList(),
                onChanged: (v) { if (v != null) setState(() => _op = v); }),
            const SizedBox(width: 12),
            Expanded(child: TextField(controller: _valCtrl,
                keyboardType: TextInputType.number,
                decoration: const InputDecoration(labelText: 'Giá trị'))),
          ]),
          const SizedBox(height: 12),
          const Text('Hành động LED:', style: TextStyle(fontWeight: FontWeight.bold)),
          SwitchListTile(title: const Text('Bật đèn'),
              value: _cmd.on,
              onChanged: (v) => setState(() => _cmd = _cmd.copyWith(on: v))),
          const SizedBox(height: 24),
          ElevatedButton(onPressed: _loading ? null : _save,
              child: _loading ? const CircularProgressIndicator() : const Text('Lưu rule')),
        ]),
      ),
    );
  }
}
