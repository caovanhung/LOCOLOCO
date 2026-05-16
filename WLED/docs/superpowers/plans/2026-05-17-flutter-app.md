# Flutter App Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Xây dựng Flutter app điều khiển LED RGB qua MQTT real-time, quản lý thiết bị, lịch hẹn, và sensor rules qua REST API.

**Architecture:** Riverpod cho state management. Dio cho HTTP requests. mqtt_client cho MQTT connection. App nhận MQTT credentials từ server sau login — không hardcode. MQTT subscribe state/status real-time khi mở DeviceDetailScreen.

**Tech Stack:** Flutter 3.x, Dart, flutter_riverpod 2.x, dio 5.x, mqtt_client 10.x, flex_color_picker, shared_preferences, go_router

---

## File Structure

```
app/
  pubspec.yaml
  lib/
    main.dart
    models/
      device.dart          — Device, LedState structs
      schedule.dart        — Schedule model
      rule.dart            — SensorRule model
      mqtt_message.dart    — envelope parse helpers
    services/
      api_service.dart     — Dio REST client (auth, devices, schedules, rules)
      mqtt_service.dart    — mqtt_client wrapper (connect, pub, sub)
    providers/
      auth_provider.dart   — login state, token, mqtt credentials
      device_provider.dart — device list, selected device state
      mqtt_provider.dart   — mqtt connection, stream per device
    screens/
      login_screen.dart
      device_list_screen.dart
      device_detail_screen.dart
      add_device_screen.dart
      schedule_list_screen.dart
      add_schedule_screen.dart
      rule_list_screen.dart
      add_rule_screen.dart
    widgets/
      led_control_panel.dart   — on/off toggle, effect, color, brightness
      sensor_panel.dart        — realtime PIR/LDR/DHT display
      online_badge.dart
  test/
    models/mqtt_message_test.dart
    services/api_service_test.dart
    providers/auth_provider_test.dart
```

---

### Task 1: Scaffold Flutter project

**Files:**
- Create: `app/pubspec.yaml`

- [ ] **Step 1: Tạo Flutter project**

```bash
flutter create --org com.locoloco --project-name locoloco app
cd app
```

- [ ] **Step 2: Cập nhật pubspec.yaml**

```yaml
name: locoloco
description: LOCOLOCO LED Controller
version: 1.0.0+1

environment:
  sdk: '>=3.0.0 <4.0.0'

dependencies:
  flutter:
    sdk: flutter
  flutter_riverpod: ^2.5.1
  riverpod_annotation: ^2.3.4
  dio: ^5.4.3
  mqtt_client: ^10.2.1
  flex_color_picker: ^3.5.1
  shared_preferences: ^2.2.3
  go_router: ^13.2.0

dev_dependencies:
  flutter_test:
    sdk: flutter
  build_runner: ^2.4.9
  riverpod_generator: ^2.4.0
  mocktail: ^1.0.3
  flutter_lints: ^3.0.0
```

- [ ] **Step 3: Install dependencies**

```bash
flutter pub get
```
Expected: tất cả packages resolve OK

- [ ] **Step 4: Verify build**

```bash
flutter build apk --debug
```
Expected: build thành công

- [ ] **Step 5: Commit**

```bash
git add app/
git commit -m "feat(app): scaffold Flutter project with dependencies"
```

---

### Task 2: Models

**Files:**
- Create: `app/lib/models/device.dart`
- Create: `app/lib/models/schedule.dart`
- Create: `app/lib/models/rule.dart`
- Create: `app/lib/models/mqtt_message.dart`
- Create: `app/test/models/mqtt_message_test.dart`

- [ ] **Step 1: Tạo models/device.dart**

```dart
// app/lib/models/device.dart
class LedState {
  final bool on;
  final int brightness;   // DP2
  final int effect;       // DP3
  final int r, g, b;      // DP4

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
```

- [ ] **Step 2: Tạo models/schedule.dart**

```dart
// app/lib/models/schedule.dart
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
```

- [ ] **Step 3: Tạo models/rule.dart**

```dart
// app/lib/models/rule.dart
class SensorRule {
  final int id;
  final String deviceId;
  final String sensorType;   // PIR, LDR, DHT_TEMP, DHT_HUM
  final String conditionOp;  // ==, !=, >, <, >=, <=
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
```

- [ ] **Step 4: Tạo models/mqtt_message.dart**

```dart
// app/lib/models/mqtt_message.dart
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
```

- [ ] **Step 5: Viết test**

```dart
// app/test/models/mqtt_message_test.dart
import 'package:flutter_test/flutter_test.dart';
import 'package:locoloco/models/mqtt_message.dart';
import 'package:locoloco/models/device.dart';

void main() {
  group('MqttEnvelope.tryParse', () {
    test('parses valid envelope', () {
      const raw = '{"v":1,"msgId":"abc","ts":1000,"devId":"esp_001","type":"rpt","data":{"dps":{"1":true,"2":180,"3":1,"4":"255,0,0"}}}';
      final env = MqttEnvelope.tryParse(raw);
      expect(env, isNotNull);
      expect(env!.devId, 'esp_001');
      expect(env.type, 'rpt');
      expect(env.data['dps']['1'], true);
    });

    test('returns null for invalid JSON', () {
      expect(MqttEnvelope.tryParse('not json'), isNull);
    });
  });

  group('LedState.fromDps', () {
    test('parses DPs correctly', () {
      final state = LedState.fromDps({'1': true, '2': 180, '3': 2, '4': '255,100,0'});
      expect(state.on, true);
      expect(state.brightness, 180);
      expect(state.r, 255);
      expect(state.g, 100);
      expect(state.b, 0);
    });

    test('handles missing DPs with defaults', () {
      final state = LedState.fromDps({});
      expect(state.on, false);
      expect(state.brightness, 200);
    });
  });
}
```

- [ ] **Step 6: Chạy test**

```bash
flutter test test/models/mqtt_message_test.dart
```
Expected: PASS (4 tests)

- [ ] **Step 7: Commit**

```bash
git add app/lib/models/ app/test/models/
git commit -m "feat(app): add Device, Schedule, Rule, MqttEnvelope models"
```

---

### Task 3: ApiService — REST client

**Files:**
- Create: `app/lib/services/api_service.dart`

- [ ] **Step 1: Tạo services/api_service.dart**

```dart
// app/lib/services/api_service.dart
import 'package:dio/dio.dart';
import '../models/device.dart';
import '../models/schedule.dart';
import '../models/rule.dart';

class MqttCredentials {
  final String host;
  final int port;
  final String username;
  final String password;
  const MqttCredentials({required this.host, required this.port,
                          required this.username, required this.password});
}

class LoginResult {
  final String token;
  final MqttCredentials mqtt;
  const LoginResult({required this.token, required this.mqtt});
}

class ApiService {
  final Dio _dio;

  ApiService(String baseUrl) : _dio = Dio(BaseOptions(
    baseUrl: baseUrl,
    connectTimeout: const Duration(seconds: 10),
    receiveTimeout: const Duration(seconds: 10),
  ));

  void setToken(String token) {
    _dio.options.headers['Authorization'] = 'Bearer $token';
  }

  Future<LoginResult> login(String username, String password) async {
    final res = await _dio.post('/api/auth/login',
        data: {'username': username, 'password': password});
    final mqttMap = res.data['mqtt'] as Map<String, dynamic>;
    return LoginResult(
      token: res.data['token'] as String,
      mqtt: MqttCredentials(
        host: mqttMap['host'] as String,
        port: mqttMap['port'] as int,
        username: mqttMap['username'] as String,
        password: mqttMap['password'] as String,
      ),
    );
  }

  Future<List<Device>> getDevices() async {
    final res = await _dio.get('/api/devices');
    return (res.data as List).map((e) => Device.fromJson(e as Map<String, dynamic>)).toList();
  }

  Future<Device> addDevice(String id, String name, String? location) async {
    final res = await _dio.post('/api/devices',
        data: {'id': id, 'name': name, 'location': location});
    return Device.fromJson(res.data as Map<String, dynamic>);
  }

  Future<void> deleteDevice(String id) async {
    await _dio.delete('/api/devices/$id');
  }

  Future<List<Schedule>> getSchedules(String deviceId) async {
    final res = await _dio.get('/api/schedules', queryParameters: {'device_id': deviceId});
    return (res.data as List).map((e) => Schedule.fromJson(e as Map<String, dynamic>)).toList();
  }

  Future<Schedule> createSchedule(String deviceId, String cronExpr,
                                   Map<String, dynamic> command) async {
    final res = await _dio.post('/api/schedules',
        data: {'device_id': deviceId, 'cron_expr': cronExpr, 'command': command});
    return Schedule.fromJson(res.data as Map<String, dynamic>);
  }

  Future<void> deleteSchedule(int id) async {
    await _dio.delete('/api/schedules/$id');
  }

  Future<List<SensorRule>> getRules(String deviceId) async {
    final res = await _dio.get('/api/rules', queryParameters: {'device_id': deviceId});
    return (res.data as List).map((e) => SensorRule.fromJson(e as Map<String, dynamic>)).toList();
  }

  Future<SensorRule> createRule(String deviceId, String sensorType,
      String conditionOp, double conditionVal, Map<String, dynamic> command) async {
    final res = await _dio.post('/api/rules', data: {
      'device_id': deviceId, 'sensor_type': sensorType,
      'condition_op': conditionOp, 'condition_val': conditionVal,
      'command': command,
    });
    return SensorRule.fromJson(res.data as Map<String, dynamic>);
  }

  Future<void> deleteRule(int id) async {
    await _dio.delete('/api/rules/$id');
  }
}
```

- [ ] **Step 2: Commit**

```bash
git add app/lib/services/api_service.dart
git commit -m "feat(app): add ApiService REST client"
```

---

### Task 4: MqttService — mqtt_client wrapper

**Files:**
- Create: `app/lib/services/mqtt_service.dart`

- [ ] **Step 1: Tạo services/mqtt_service.dart**

```dart
// app/lib/services/mqtt_service.dart
import 'dart:async';
import 'dart:convert';
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';
import '../models/mqtt_message.dart';

class MqttService {
  MqttServerClient? _client;
  final _messageController = StreamController<MqttEnvelope>.broadcast();

  Stream<MqttEnvelope> get messages => _messageController.stream;

  Future<void> connect({
    required String host, required int port,
    required String username, required String password,
  }) async {
    _client = MqttServerClient.withPort(host, 'loco-app-${DateTime.now().millisecondsSinceEpoch}', port);
    _client!.secure = true;
    _client!.onBadCertificate = (_) => true; // accept self-signed
    _client!.keepAlivePeriod = 120;
    _client!.logging(on: false);

    final connMsg = MqttConnectMessage()
        .withClientIdentifier(_client!.clientIdentifier)
        .authenticateAs(username, password)
        .startClean();
    _client!.connectionMessage = connMsg;

    await _client!.connect();

    _client!.updates?.listen((List<MqttReceivedMessage<MqttMessage>> events) {
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
}
```

- [ ] **Step 2: Commit**

```bash
git add app/lib/services/mqtt_service.dart
git commit -m "feat(app): add MqttService with TLS and envelope publish"
```

---

### Task 5: Providers — auth + devices + mqtt

**Files:**
- Create: `app/lib/providers/auth_provider.dart`
- Create: `app/lib/providers/device_provider.dart`
- Create: `app/lib/providers/mqtt_provider.dart`

- [ ] **Step 1: Tạo providers/auth_provider.dart**

```dart
// app/lib/providers/auth_provider.dart
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../services/api_service.dart';

const _serverUrl = 'https://your-vps.com:3000'; // đổi theo VPS

final apiServiceProvider = Provider<ApiService>((ref) => ApiService(_serverUrl));

class AuthState {
  final bool isLoading;
  final String? token;
  final MqttCredentials? mqttCreds;
  final String? error;

  const AuthState({this.isLoading = false, this.token, this.mqttCreds, this.error});

  bool get isLoggedIn => token != null;
  AuthState copyWith({bool? isLoading, String? token, MqttCredentials? mqttCreds, String? error}) =>
    AuthState(
      isLoading: isLoading ?? this.isLoading,
      token: token ?? this.token,
      mqttCreds: mqttCreds ?? this.mqttCreds,
      error: error,
    );
}

class AuthNotifier extends StateNotifier<AuthState> {
  final ApiService _api;
  AuthNotifier(this._api) : super(const AuthState());

  Future<void> login(String username, String password) async {
    state = state.copyWith(isLoading: true, error: null);
    try {
      final result = await _api.login(username, password);
      _api.setToken(result.token);
      state = state.copyWith(isLoading: false, token: result.token, mqttCreds: result.mqtt);
    } catch (e) {
      state = state.copyWith(isLoading: false, error: e.toString());
    }
  }

  void logout() {
    state = const AuthState();
  }
}

final authProvider = StateNotifierProvider<AuthNotifier, AuthState>((ref) {
  return AuthNotifier(ref.watch(apiServiceProvider));
});
```

- [ ] **Step 2: Tạo providers/mqtt_provider.dart**

```dart
// app/lib/providers/mqtt_provider.dart
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../services/mqtt_service.dart';
import '../models/mqtt_message.dart';

final mqttServiceProvider = Provider<MqttService>((ref) => MqttService());

final mqttConnectedProvider = StateProvider<bool>((ref) => false);

// Stream messages filtered by devId
final deviceMessageProvider = StreamProvider.family<MqttEnvelope, String>((ref, deviceId) {
  final mqtt = ref.watch(mqttServiceProvider);
  return mqtt.messages.where((e) => e.devId == deviceId);
});
```

- [ ] **Step 3: Tạo providers/device_provider.dart**

```dart
// app/lib/providers/device_provider.dart
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../models/device.dart';
import '../services/api_service.dart';
import 'auth_provider.dart';

final deviceListProvider = FutureProvider<List<Device>>((ref) async {
  final api = ref.watch(apiServiceProvider);
  return api.getDevices();
});

final selectedDeviceStateProvider = StateProvider.family<LedState?, String>((ref, deviceId) => null);
final deviceOnlineProvider = StateProvider.family<bool, String>((ref, deviceId) => false);
```

- [ ] **Step 4: Commit**

```bash
git add app/lib/providers/
git commit -m "feat(app): add auth, mqtt, and device Riverpod providers"
```

---

### Task 6: LoginScreen

**Files:**
- Create: `app/lib/screens/login_screen.dart`

- [ ] **Step 1: Tạo screens/login_screen.dart**

```dart
// app/lib/screens/login_screen.dart
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../providers/auth_provider.dart';
import '../providers/mqtt_provider.dart';
import '../services/mqtt_service.dart';
import 'device_list_screen.dart';

class LoginScreen extends ConsumerStatefulWidget {
  const LoginScreen({super.key});
  @override
  ConsumerState<LoginScreen> createState() => _LoginScreenState();
}

class _LoginScreenState extends ConsumerState<LoginScreen> {
  final _userCtrl = TextEditingController();
  final _passCtrl = TextEditingController();

  Future<void> _login() async {
    await ref.read(authProvider.notifier).login(_userCtrl.text, _passCtrl.text);
    final auth = ref.read(authProvider);
    if (auth.isLoggedIn && auth.mqttCreds != null && mounted) {
      final mqtt = ref.read(mqttServiceProvider);
      await mqtt.connect(
        host: auth.mqttCreds!.host,
        port: auth.mqttCreds!.port,
        username: auth.mqttCreds!.username,
        password: auth.mqttCreds!.password,
      );
      ref.read(mqttConnectedProvider.notifier).state = mqtt.isConnected;
      if (mounted) {
        Navigator.of(context).pushReplacement(
          MaterialPageRoute(builder: (_) => const DeviceListScreen()));
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    final auth = ref.watch(authProvider);
    return Scaffold(
      appBar: AppBar(title: const Text('LOCOLOCO')),
      body: Padding(
        padding: const EdgeInsets.all(24),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            TextField(controller: _userCtrl, decoration: const InputDecoration(labelText: 'Username')),
            const SizedBox(height: 12),
            TextField(controller: _passCtrl, obscureText: true,
                      decoration: const InputDecoration(labelText: 'Password')),
            const SizedBox(height: 24),
            if (auth.error != null) Text(auth.error!, style: const TextStyle(color: Colors.red)),
            ElevatedButton(
              onPressed: auth.isLoading ? null : _login,
              child: auth.isLoading
                  ? const SizedBox(width: 20, height: 20, child: CircularProgressIndicator(strokeWidth: 2))
                  : const Text('Login'),
            ),
          ],
        ),
      ),
    );
  }
}
```

- [ ] **Step 2: Cập nhật main.dart**

```dart
// app/lib/main.dart
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'screens/login_screen.dart';

void main() {
  runApp(const ProviderScope(child: LocoApp()));
}

class LocoApp extends StatelessWidget {
  const LocoApp({super.key});
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'LOCOLOCO',
      theme: ThemeData(colorSchemeSeed: Colors.deepOrange, useMaterial3: true),
      home: const LoginScreen(),
    );
  }
}
```

- [ ] **Step 3: Commit**

```bash
git add app/lib/screens/login_screen.dart app/lib/main.dart
git commit -m "feat(app): add LoginScreen with JWT auth and MQTT connect"
```

---

### Task 7: DeviceListScreen + AddDeviceScreen

**Files:**
- Create: `app/lib/screens/device_list_screen.dart`
- Create: `app/lib/screens/add_device_screen.dart`

- [ ] **Step 1: Tạo screens/device_list_screen.dart**

```dart
// app/lib/screens/device_list_screen.dart
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../providers/device_provider.dart';
import '../models/device.dart';
import 'device_detail_screen.dart';
import 'add_device_screen.dart';

class DeviceListScreen extends ConsumerWidget {
  const DeviceListScreen({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final devicesAsync = ref.watch(deviceListProvider);
    return Scaffold(
      appBar: AppBar(title: const Text('Thiết bị')),
      floatingActionButton: FloatingActionButton(
        onPressed: () async {
          await Navigator.push(context, MaterialPageRoute(builder: (_) => const AddDeviceScreen()));
          ref.invalidate(deviceListProvider);
        },
        child: const Icon(Icons.add),
      ),
      body: devicesAsync.when(
        loading: () => const Center(child: CircularProgressIndicator()),
        error: (e, _) => Center(child: Text('Lỗi: $e')),
        data: (devices) => devices.isEmpty
            ? const Center(child: Text('Chưa có thiết bị nào'))
            : ListView.builder(
                itemCount: devices.length,
                itemBuilder: (ctx, i) {
                  final d = devices[i];
                  return ListTile(
                    leading: Icon(Icons.lightbulb,
                        color: d.online ? Colors.amber : Colors.grey),
                    title: Text(d.name),
                    subtitle: Text(d.location ?? d.id),
                    trailing: const Icon(Icons.chevron_right),
                    onTap: () => Navigator.push(context,
                        MaterialPageRoute(builder: (_) => DeviceDetailScreen(device: d))),
                  );
                },
              ),
      ),
    );
  }
}
```

- [ ] **Step 2: Tạo screens/add_device_screen.dart**

```dart
// app/lib/screens/add_device_screen.dart
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../providers/auth_provider.dart';

class AddDeviceScreen extends ConsumerStatefulWidget {
  const AddDeviceScreen({super.key});
  @override
  ConsumerState<AddDeviceScreen> createState() => _AddDeviceScreenState();
}

class _AddDeviceScreenState extends ConsumerState<AddDeviceScreen> {
  final _idCtrl   = TextEditingController();
  final _nameCtrl = TextEditingController();
  final _locCtrl  = TextEditingController();
  bool _loading = false;

  Future<void> _save() async {
    if (_idCtrl.text.isEmpty || _nameCtrl.text.isEmpty) return;
    setState(() => _loading = true);
    try {
      final api = ref.read(apiServiceProvider);
      await api.addDevice(_idCtrl.text.trim(), _nameCtrl.text.trim(),
                          _locCtrl.text.isEmpty ? null : _locCtrl.text.trim());
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
      appBar: AppBar(title: const Text('Thêm thiết bị')),
      body: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(children: [
          TextField(controller: _idCtrl,   decoration: const InputDecoration(labelText: 'Device ID (vd: esp_001)')),
          TextField(controller: _nameCtrl, decoration: const InputDecoration(labelText: 'Tên thiết bị')),
          TextField(controller: _locCtrl,  decoration: const InputDecoration(labelText: 'Vị trí (tùy chọn)')),
          const SizedBox(height: 24),
          ElevatedButton(
            onPressed: _loading ? null : _save,
            child: _loading ? const CircularProgressIndicator() : const Text('Lưu'),
          ),
        ]),
      ),
    );
  }
}
```

- [ ] **Step 3: Commit**

```bash
git add app/lib/screens/device_list_screen.dart app/lib/screens/add_device_screen.dart
git commit -m "feat(app): add DeviceListScreen and AddDeviceScreen"
```

---

### Task 8: DeviceDetailScreen + LedControlPanel

**Files:**
- Create: `app/lib/screens/device_detail_screen.dart`
- Create: `app/lib/widgets/led_control_panel.dart`
- Create: `app/lib/widgets/online_badge.dart`
- Create: `app/lib/widgets/sensor_panel.dart`

- [ ] **Step 1: Tạo widgets/online_badge.dart**

```dart
// app/lib/widgets/online_badge.dart
import 'package:flutter/material.dart';

class OnlineBadge extends StatelessWidget {
  final bool online;
  const OnlineBadge({super.key, required this.online});
  @override
  Widget build(BuildContext context) => Row(
    mainAxisSize: MainAxisSize.min,
    children: [
      Icon(Icons.circle, size: 10, color: online ? Colors.green : Colors.red),
      const SizedBox(width: 4),
      Text(online ? 'Online' : 'Offline',
           style: TextStyle(color: online ? Colors.green : Colors.red, fontSize: 12)),
    ],
  );
}
```

- [ ] **Step 2: Tạo widgets/sensor_panel.dart**

```dart
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
```

- [ ] **Step 3: Tạo widgets/led_control_panel.dart**

```dart
// app/lib/widgets/led_control_panel.dart
import 'package:flutter/material.dart';
import 'package:flex_color_picker/flex_color_picker.dart';
import '../models/device.dart';

const effectNames = ['OFF', 'SOLID', 'BLINK', 'FADE', 'CHASE'];

class LedControlPanel extends StatelessWidget {
  final LedState state;
  final void Function(LedState) onChanged;

  const LedControlPanel({super.key, required this.state, required this.onChanged});

  @override
  Widget build(BuildContext context) {
    return Column(children: [
      SwitchListTile(
        title: const Text('Bật/Tắt'),
        value: state.on,
        onChanged: (v) => onChanged(state.copyWith(on: v)),
      ),
      ListTile(
        title: const Text('Hiệu ứng'),
        trailing: DropdownButton<int>(
          value: state.effect.clamp(0, 4),
          items: List.generate(5, (i) =>
              DropdownMenuItem(value: i, child: Text(effectNames[i]))),
          onChanged: (v) { if (v != null) onChanged(state.copyWith(effect: v)); },
        ),
      ),
      ListTile(
        title: Text('Độ sáng: ${state.brightness}'),
        subtitle: Slider(
          value: state.brightness.toDouble(),
          min: 0, max: 255,
          onChanged: (v) => onChanged(state.copyWith(brightness: v.toInt())),
        ),
      ),
      ListTile(
        title: const Text('Màu sắc'),
        trailing: ColorIndicator(
          width: 44, height: 44, borderRadius: 22,
          color: Color.fromRGBO(state.r, state.g, state.b, 1),
          onSelectFocus: false,
          onSelect: () async {
            Color picked = Color.fromRGBO(state.r, state.g, state.b, 1);
            final ok = await ColorPicker(
              color: picked,
              onColorChanged: (c) => picked = c,
              title: const Text('Chọn màu'),
              pickersEnabled: const {ColorPickerType.wheel: true},
            ).showPickerDialog(context);
            if (ok) {
              onChanged(state.copyWith(r: picked.red, g: picked.green, b: picked.blue));
            }
          },
        ),
      ),
    ]);
  }
}
```

- [ ] **Step 4: Tạo screens/device_detail_screen.dart**

```dart
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
```

- [ ] **Step 5: Commit**

```bash
git add app/lib/screens/device_detail_screen.dart app/lib/widgets/
git commit -m "feat(app): add DeviceDetailScreen with LED control, sensor panel, and MQTT real-time"
```

---

### Task 9: ScheduleListScreen + AddScheduleScreen

**Files:**
- Create: `app/lib/screens/schedule_list_screen.dart`
- Create: `app/lib/screens/add_schedule_screen.dart`
- Create: `app/lib/screens/rule_list_screen.dart`
- Create: `app/lib/screens/add_rule_screen.dart`

- [ ] **Step 1: Tạo screens/schedule_list_screen.dart**

```dart
// app/lib/screens/schedule_list_screen.dart
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../providers/auth_provider.dart';
import '../models/schedule.dart';
import 'add_schedule_screen.dart';

final _schedulesProvider = FutureProvider.family<List<Schedule>, String>((ref, deviceId) async {
  return ref.watch(apiServiceProvider).getSchedules(deviceId);
});

class ScheduleListScreen extends ConsumerWidget {
  final String deviceId;
  const ScheduleListScreen({super.key, required this.deviceId});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final asyncSchedules = ref.watch(_schedulesProvider(deviceId));
    return Scaffold(
      floatingActionButton: FloatingActionButton(
        mini: true,
        onPressed: () async {
          await Navigator.push(context,
              MaterialPageRoute(builder: (_) => AddScheduleScreen(deviceId: deviceId)));
          ref.invalidate(_schedulesProvider(deviceId));
        },
        child: const Icon(Icons.add),
      ),
      body: asyncSchedules.when(
        loading: () => const Center(child: CircularProgressIndicator()),
        error: (e, _) => Center(child: Text('Lỗi: $e')),
        data: (schedules) => schedules.isEmpty
            ? const Center(child: Text('Chưa có lịch hẹn'))
            : ListView.builder(
                itemCount: schedules.length,
                itemBuilder: (ctx, i) {
                  final s = schedules[i];
                  return ListTile(
                    leading: const Icon(Icons.schedule),
                    title: Text(s.cronExpr),
                    subtitle: Text('Effect: ${s.command['3'] ?? 0}  Bri: ${s.command['2'] ?? 200}'),
                    trailing: IconButton(
                      icon: const Icon(Icons.delete, color: Colors.red),
                      onPressed: () async {
                        await ref.read(apiServiceProvider).deleteSchedule(s.id);
                        ref.invalidate(_schedulesProvider(deviceId));
                      },
                    ),
                  );
                },
              ),
      ),
    );
  }
}
```

- [ ] **Step 2: Tạo screens/add_schedule_screen.dart**

```dart
// app/lib/screens/add_schedule_screen.dart
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../models/device.dart';
import '../providers/auth_provider.dart';

class AddScheduleScreen extends ConsumerStatefulWidget {
  final String deviceId;
  const AddScheduleScreen({super.key, required this.deviceId});
  @override
  ConsumerState<AddScheduleScreen> createState() => _AddScheduleScreenState();
}

class _AddScheduleScreenState extends ConsumerState<AddScheduleScreen> {
  final _cronCtrl = TextEditingController(text: '0 8 * * *');
  LedState _cmd = const LedState(on: true, brightness: 200, effect: 1);
  bool _loading = false;

  Future<void> _save() async {
    setState(() => _loading = true);
    try {
      await ref.read(apiServiceProvider).createSchedule(
        widget.deviceId, _cronCtrl.text.trim(), _cmd.toDps());
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
      appBar: AppBar(title: const Text('Thêm lịch hẹn')),
      body: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(children: [
          TextField(controller: _cronCtrl,
              decoration: const InputDecoration(
                labelText: 'Cron expression',
                helperText: 'VD: 0 8 * * 1-5 (8 giờ sáng, thứ 2-6)')),
          const SizedBox(height: 12),
          const Text('Lệnh LED:', style: TextStyle(fontWeight: FontWeight.bold)),
          SwitchListTile(title: const Text('Bật đèn'),
              value: _cmd.on,
              onChanged: (v) => setState(() => _cmd = _cmd.copyWith(on: v))),
          ListTile(
            title: Text('Độ sáng: ${_cmd.brightness}'),
            subtitle: Slider(
              value: _cmd.brightness.toDouble(), min: 0, max: 255,
              onChanged: (v) => setState(() => _cmd = _cmd.copyWith(brightness: v.toInt())),
            ),
          ),
          const SizedBox(height: 24),
          ElevatedButton(onPressed: _loading ? null : _save,
              child: _loading ? const CircularProgressIndicator() : const Text('Lưu lịch')),
        ]),
      ),
    );
  }
}
```

- [ ] **Step 3: Tạo screens/rule_list_screen.dart**

```dart
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
```

- [ ] **Step 4: Tạo screens/add_rule_screen.dart**

```dart
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
```

- [ ] **Step 5: Chạy tất cả Flutter tests**

```bash
flutter test
```
Expected: PASS

- [ ] **Step 6: Build APK debug**

```bash
flutter build apk --debug
```
Expected: `BUILD SUCCESSFUL`

- [ ] **Step 7: Commit**

```bash
git add app/lib/screens/ app/lib/widgets/
git commit -m "feat(app): complete all screens - schedule, rule, sensor panel"
```

---

### Task 10: End-to-end smoke test

- [ ] **Step 1: Kết nối thiết bị ESP8266 và server**

Đảm bảo:
- ESP8266 V1.0.3 đã flash, Serial monitor hiện `MQTT connected`
- Server chạy, log hiện `MQTT connected`
- Mosquitto chạy trên VPS

- [ ] **Step 2: Cài và chạy app trên thiết bị Android**

```bash
flutter run --release
```

- [ ] **Step 3: Test flow hoàn chỉnh**

Thực hiện theo thứ tự:
1. Login với username/password
2. Thêm device `esp_001`
3. Vào DeviceDetailScreen → LED badge hiện Online
4. Toggle ON → LED strip bật ngay
5. Đổi hiệu ứng BLINK → LED strip nhấp nháy
6. Thêm lịch hẹn (cron `* * * * *` để test ngay) → sau 1 phút LED tự chạy
7. Thêm sensor rule: PIR == 1 → LED CHASE → kích PIR → LED chuyển Chase
8. Kiểm tra SensorPanel hiện data PIR real-time

Expected: tất cả flow hoạt động end-to-end.

- [ ] **Step 4: Final commit**

```bash
git add .
git commit -m "feat: LOCOLOCO IoT system Phase 3 complete - Flutter app"
```
