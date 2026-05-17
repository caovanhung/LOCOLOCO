// app/lib/screens/device_detail_screen.dart
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../models/device.dart';
import '../models/mqtt_message.dart';
import '../providers/mqtt_provider.dart';
import '../providers/device_provider.dart';
import '../services/mqtt_service.dart';
import '../widgets/led_control_panel.dart';
import '../widgets/online_badge.dart';
import '../widgets/sensor_panel.dart';
import 'schedule_list_screen.dart';
import 'rule_list_screen.dart';

class DeviceDetailScreen extends ConsumerStatefulWidget {
  final Device device;
  const DeviceDetailScreen({super.key, required this.device});
  @override
  ConsumerState<DeviceDetailScreen> createState() => _DeviceDetailScreenState();
}

class _DeviceDetailScreenState extends ConsumerState<DeviceDetailScreen> {
  MqttEnvelope? _lastSensor;

  @override
  void initState() {
    super.initState();
    final mqtt = ref.read(mqttServiceProvider);
    mqtt.subscribe('loco/v1/${widget.device.id}/rpt/state');
    mqtt.subscribe('loco/v1/${widget.device.id}/rpt/sensor');
    mqtt.subscribe('loco/v1/${widget.device.id}/evt/status');
  }

  @override
  void dispose() {
    final mqtt = ref.read(mqttServiceProvider);
    mqtt.unsubscribe('loco/v1/${widget.device.id}/rpt/state');
    mqtt.unsubscribe('loco/v1/${widget.device.id}/rpt/sensor');
    mqtt.unsubscribe('loco/v1/${widget.device.id}/evt/status');
    super.dispose();
  }

  void _sendLedCommand(LedState newState) {
    final mqtt = ref.read(mqttServiceProvider);
    mqtt.publishLedCommand(widget.device.id, newState.toDps());
    ref.read(selectedDeviceStateProvider(widget.device.id).notifier).state = newState;
  }

  @override
  Widget build(BuildContext context) {
    final ledState = ref.watch(selectedDeviceStateProvider(widget.device.id))
        ?? widget.device.lastState ?? const LedState();
    final online = ref.watch(deviceOnlineProvider(widget.device.id));

    ref.listen(deviceMessageProvider(widget.device.id), (_, next) {
      next.whenData((env) {
        if (env.type == 'rpt' && env.data.containsKey('dps')) {
          ref.read(selectedDeviceStateProvider(widget.device.id).notifier).state =
              LedState.fromDps(Map<String, dynamic>.from(env.data['dps'] as Map));
        }
        if (env.type == 'rpt' && env.data.containsKey('sensors')) {
          setState(() => _lastSensor = env);
        }
        if (env.type == 'evt' && env.data.containsKey('online')) {
          ref.read(deviceOnlineProvider(widget.device.id).notifier).state =
              env.data['online'] as bool;
        }
      });
    });

    return DefaultTabController(
      length: 3,
      child: Scaffold(
        appBar: AppBar(
          title: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(widget.device.name),
              OnlineBadge(online: online),
            ],
          ),
          bottom: const TabBar(tabs: [
            Tab(text: 'Điều khiển'),
            Tab(text: 'Lịch hẹn'),
            Tab(text: 'Sensor Rules'),
          ]),
        ),
        body: TabBarView(children: [
          SingleChildScrollView(child: Column(children: [
            LedControlPanel(state: ledState, onChanged: _sendLedCommand),
            const Divider(),
            const Padding(padding: EdgeInsets.all(8), child: Text('Sensor', style: TextStyle(fontWeight: FontWeight.bold))),
            SensorPanel(lastSensor: _lastSensor),
          ])),
          ScheduleListScreen(deviceId: widget.device.id),
          RuleListScreen(deviceId: widget.device.id),
        ]),
      ),
    );
  }
}
