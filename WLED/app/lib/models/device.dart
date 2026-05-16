class LedState {
  final bool on;
  final int brightness;
  final int effect;
  final int r, g, b;

  const LedState({
    this.on = false,
    this.brightness = 200,
    this.effect = 0,
    this.r = 255,
    this.g = 255,
    this.b = 255,
  });

  factory LedState.fromDps(Map<String, dynamic> dps) {
    final colorStr = dps['4'] as String? ?? '255,255,255';
    final parts = colorStr.split(',');
    return LedState(
      on: dps['1'] as bool? ?? false,
      brightness: (dps['2'] as num?)?.toInt() ?? 200,
      effect: (dps['3'] as num?)?.toInt() ?? 0,
      r: int.tryParse(parts.isNotEmpty ? parts[0] : '255') ?? 255,
      g: int.tryParse(parts.length > 1 ? parts[1] : '255') ?? 255,
      b: int.tryParse(parts.length > 2 ? parts[2] : '255') ?? 255,
    );
  }

  Map<String, dynamic> toDps() => {
    '1': on,
    '2': brightness,
    '3': effect,
    '4': '$r,$g,$b',
  };

  LedState copyWith({bool? on, int? brightness, int? effect, int? r, int? g, int? b}) =>
    LedState(
      on: on ?? this.on,
      brightness: brightness ?? this.brightness,
      effect: effect ?? this.effect,
      r: r ?? this.r, g: g ?? this.g, b: b ?? this.b,
    );
}

class Device {
  final String id;
  final String name;
  final String? location;
  final bool online;
  final LedState? lastState;

  const Device({
    required this.id,
    required this.name,
    this.location,
    this.online = false,
    this.lastState,
  });

  factory Device.fromJson(Map<String, dynamic> json) => Device(
    id: json['id'] as String,
    name: json['name'] as String,
    location: json['location'] as String?,
    online: json['online'] as bool? ?? false,
    lastState: json['last_state'] != null
        ? LedState.fromDps(Map<String, dynamic>.from(json['last_state'] as Map))
        : null,
  );

  Device copyWith({bool? online, LedState? lastState}) => Device(
    id: id, name: name, location: location,
    online: online ?? this.online,
    lastState: lastState ?? this.lastState,
  );
}
