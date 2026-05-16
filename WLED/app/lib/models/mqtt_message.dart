import 'dart:convert';

class MqttEnvelope {
  final int v;
  final String msgId;
  final int ts;
  final String devId;
  final String type;
  final Map<String, dynamic> data;

  const MqttEnvelope({
    required this.v, required this.msgId, required this.ts,
    required this.devId, required this.type, required this.data,
  });

  factory MqttEnvelope.fromJson(Map<String, dynamic> json) => MqttEnvelope(
    v: json['v'] as int,
    msgId: json['msgId'] as String,
    ts: (json['ts'] as num).toInt(),
    devId: json['devId'] as String,
    type: json['type'] as String,
    data: Map<String, dynamic>.from(json['data'] as Map),
  );

  static MqttEnvelope? tryParse(String raw) {
    try {
      return MqttEnvelope.fromJson(jsonDecode(raw) as Map<String, dynamic>);
    } catch (_) { return null; }
  }
}
