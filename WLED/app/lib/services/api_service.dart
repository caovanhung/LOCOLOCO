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
