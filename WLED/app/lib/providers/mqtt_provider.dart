import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../services/mqtt_service.dart';
import '../models/mqtt_message.dart';

final mqttServiceProvider = Provider<MqttService>((ref) => MqttService());

final mqttConnectedProvider = StateProvider<bool>((ref) => false);

final deviceMessageProvider = StreamProvider.family<MqttEnvelope, String>((ref, deviceId) {
  final mqtt = ref.watch(mqttServiceProvider);
  return mqtt.messages.where((e) => e.devId == deviceId);
});
