// app/lib/widgets/sensor_panel.dart
import 'package:flutter/material.dart';
import '../models/mqtt_message.dart';

class SensorPanel extends StatelessWidget {
  final MqttEnvelope? lastSensor;
  const SensorPanel({super.key, this.lastSensor});

  @override
  Widget build(BuildContext context) {
    final sensors = lastSensor?.data['sensors'] as Map<String, dynamic>?;
    if (sensors == null) return const ListTile(title: Text('Chờ dữ liệu sensor...'));
    return Column(
      children: [
        if (sensors.containsKey('pir'))
          ListTile(leading: const Icon(Icons.person_search),
                   title: const Text('PIR'), trailing: Text('${sensors['pir']}')),
        if (sensors.containsKey('ldr'))
          ListTile(leading: const Icon(Icons.wb_sunny),
                   title: const Text('LDR'), trailing: Text('${sensors['ldr']}')),
        if (sensors.containsKey('temp'))
          ListTile(leading: const Icon(Icons.thermostat),
                   title: const Text('Nhiệt độ'), trailing: Text('${sensors['temp']}°C')),
        if (sensors.containsKey('hum'))
          ListTile(leading: const Icon(Icons.water_drop),
                   title: const Text('Độ ẩm'), trailing: Text('${sensors['hum']}%')),
      ],
    );
  }
}
