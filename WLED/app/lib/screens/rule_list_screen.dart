// app/lib/screens/rule_list_screen.dart
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../providers/auth_provider.dart';
import '../models/rule.dart';
import 'add_rule_screen.dart';

final _rulesProvider = FutureProvider.family<List<SensorRule>, String>((ref, deviceId) async {
  return ref.watch(apiServiceProvider).getRules(deviceId);
});

class RuleListScreen extends ConsumerWidget {
  final String deviceId;
  const RuleListScreen({super.key, required this.deviceId});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final asyncRules = ref.watch(_rulesProvider(deviceId));
    return Scaffold(
      floatingActionButton: FloatingActionButton(
        mini: true,
        onPressed: () async {
          await Navigator.push(context,
              MaterialPageRoute(builder: (_) => AddRuleScreen(deviceId: deviceId)));
          ref.invalidate(_rulesProvider(deviceId));
        },
        child: const Icon(Icons.add),
      ),
      body: asyncRules.when(
        loading: () => const Center(child: CircularProgressIndicator()),
        error: (e, _) => Center(child: Text('Lỗi: $e')),
        data: (rules) => rules.isEmpty
            ? const Center(child: Text('Chưa có sensor rule'))
            : ListView.builder(
                itemCount: rules.length,
                itemBuilder: (ctx, i) {
                  final r = rules[i];
                  return ListTile(
                    leading: const Icon(Icons.sensors),
                    title: Text('${r.sensorType} ${r.conditionOp} ${r.conditionVal}'),
                    subtitle: Text('→ Effect: ${r.command['3'] ?? 0}'),
                    trailing: IconButton(
                      icon: const Icon(Icons.delete, color: Colors.red),
                      onPressed: () async {
                        await ref.read(apiServiceProvider).deleteRule(r.id);
                        ref.invalidate(_rulesProvider(deviceId));
                      },
                    ),
                  );
                },
              ),
      ),
    );
  }
}
