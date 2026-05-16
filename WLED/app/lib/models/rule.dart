class SensorRule {
  final int id;
  final String deviceId;
  final String sensorType;
  final String conditionOp;
  final double conditionVal;
  final Map<String, dynamic> command;
  final bool enabled;

  const SensorRule({
    required this.id,
    required this.deviceId,
    required this.sensorType,
    required this.conditionOp,
    required this.conditionVal,
    required this.command,
    required this.enabled,
  });

  factory SensorRule.fromJson(Map<String, dynamic> json) => SensorRule(
    id: json['id'] as int,
    deviceId: json['device_id'] as String,
    sensorType: json['sensor_type'] as String,
    conditionOp: json['condition_op'] as String,
    conditionVal: (json['condition_val'] as num).toDouble(),
    command: Map<String, dynamic>.from(json['command'] as Map),
    enabled: json['enabled'] as bool? ?? true,
  );
}
