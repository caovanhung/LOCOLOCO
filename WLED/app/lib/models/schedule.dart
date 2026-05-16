class Schedule {
  final int id;
  final String deviceId;
  final String cronExpr;
  final Map<String, dynamic> command;
  final bool enabled;

  const Schedule({
    required this.id,
    required this.deviceId,
    required this.cronExpr,
    required this.command,
    required this.enabled,
  });

  factory Schedule.fromJson(Map<String, dynamic> json) => Schedule(
    id: json['id'] as int,
    deviceId: json['device_id'] as String,
    cronExpr: json['cron_expr'] as String,
    command: Map<String, dynamic>.from(json['command'] as Map),
    enabled: json['enabled'] as bool? ?? true,
  );
}
