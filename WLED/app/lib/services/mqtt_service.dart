import 'dart:async';
import 'dart:convert';
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';
import '../models/mqtt_message.dart';

class MqttService {
  MqttServerClient? _client;
  final _messageController = StreamController<MqttEnvelope>.broadcast();
  StreamSubscription? _updatesSub;

  Stream<MqttEnvelope> get messages => _messageController.stream;

  Future<void> connect({
    required String host, required int port,
    required String username, required String password,
  }) async {
    await _updatesSub?.cancel();
    _updatesSub = null;
    _client?.disconnect();

    _client = MqttServerClient.withPort(host, 'loco-app-${DateTime.now().millisecondsSinceEpoch}', port);
    _client!.secure = true;
    _client!.onBadCertificate = (_) => true;
    _client!.keepAlivePeriod = 120;
    _client!.logging(on: false);

    final connMsg = MqttConnectMessage()
        .withClientIdentifier(_client!.clientIdentifier)
        .authenticateAs(username, password)
        .startClean();
    _client!.connectionMessage = connMsg;

    try {
      await _client!.connect();
    } catch (e) {
      _client?.disconnect();
      _client = null;
      rethrow;
    }

    _updatesSub = _client!.updates?.listen((List<MqttReceivedMessage<MqttMessage>> events) {
      for (final event in events) {
        final pubMsg = event.payload as MqttPublishMessage;
        final raw = MqttPublishPayload.bytesToStringAsString(pubMsg.payload.message);
        final env = MqttEnvelope.tryParse(raw);
        if (env != null) _messageController.add(env);
      }
    });
  }

  void subscribe(String topic) {
    _client?.subscribe(topic, MqttQos.atMostOnce);
  }

  void unsubscribe(String topic) {
    _client?.unsubscribeStringTopic(topic);
  }

  void publish(String topic, Map<String, dynamic> envelope) {
    final builder = MqttClientPayloadBuilder();
    builder.addString(jsonEncode(envelope));
    _client?.publishMessage(topic, MqttQos.atLeastOnce, builder.payload!);
  }

  void publishLedCommand(String deviceId, Map<String, dynamic> dps) {
    final envelope = {
      'v': 1,
      'msgId': DateTime.now().millisecondsSinceEpoch.toRadixString(16),
      'ts': DateTime.now().millisecondsSinceEpoch,
      'devId': deviceId,
      'type': 'cmd',
      'data': {'dps': dps},
    };
    publish('loco/v1/$deviceId/cmd/led', envelope);
  }

  void disconnect() {
    _client?.disconnect();
  }

  bool get isConnected =>
      _client?.connectionStatus?.state == MqttConnectionState.connected;

  Future<void> dispose() async {
    await _updatesSub?.cancel();
    _client?.disconnect();
    await _messageController.close();
  }
}
