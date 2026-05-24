import 'package:flutter/foundation.dart';
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

class ApiException implements Exception {
  final String message;
  final int? statusCode;
  const ApiException(this.message, {this.statusCode});
  @override
  String toString() => statusCode != null ? 'ApiException($statusCode): $message' : 'ApiException: $message';
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

  Future<T> _call<T>(Future<T> Function() fn) async {
    try {
      return await fn();
    } on DioException catch (e) {
      final code = e.response?.statusCode;
      final data = e.response?.data;
      final msg = (data is Map ? data['error'] as String? : null) ?? e.message ?? 'Network error';
      throw ApiException(msg, statusCode: code);
    }
  }

  Future<LoginResult> login(String username, String password) => _call(() async {
    debugPrint('[API] login sending: username="$username" password="$password"');
    final res = await _dio.post('/api/auth/login',
        data: {'username': username, 'password': password});
    debugPrint('[API] login response: ${res.data}');
    final mqttMap = res.data['mqtt'] as Map<String, dynamic>;
    debugPrint('[API] port type=${mqttMap['port'].runtimeType} value=${mqttMap['port']}');
    return LoginResult(
      token: res.data['token'] as String,
      mqtt: MqttCredentials(
        host: mqttMap['host'] as String,
        port: int.parse(mqttMap['port'].toString()),
        username: mqttMap['username'] as String,
        password: mqttMap['password'] as String,
      ),
    );
  });

  Future<void> register(String username, String email, String password) => _call(() async {
    await _dio.post('/api/auth/register',
        data: {'username': username, 'email': email, 'password': password});
  });

  Future<void> resendVerification(String email) => _call(() async {
    await _dio.post('/api/auth/resend-verification', data: {'email': email});
  });

  Future<List<Device>> getDevices() => _call(() async {
    final res = await _dio.get('/api/devices');
    return (res.data as List).map((e) => Device.fromJson(e as Map<String, dynamic>)).toList();
  });

  Future<Device> addDevice(String id, String name, String? location) => _call(() async {
    final res = await _dio.post('/api/devices',
        data: {'id': id, 'name': name, 'location': location});
    return Device.fromJson(res.data as Map<String, dynamic>);
  });

  Future<void> deleteDevice(String id) => _call(() => _dio.delete('/api/devices/$id'));

  Future<List<Schedule>> getSchedules(String deviceId) => _call(() async {
    final res = await _dio.get('/api/schedules', queryParameters: {'device_id': deviceId});
    return (res.data as List).map((e) => Schedule.fromJson(e as Map<String, dynamic>)).toList();
  });

  Future<Schedule> createSchedule(String deviceId, String cronExpr,
                                   Map<String, dynamic> command) => _call(() async {
    final res = await _dio.post('/api/schedules',
        data: {'device_id': deviceId, 'cron_expr': cronExpr, 'command': command});
    return Schedule.fromJson(res.data as Map<String, dynamic>);
  });

  Future<void> deleteSchedule(int id) => _call(() => _dio.delete('/api/schedules/$id'));

  Future<List<SensorRule>> getRules(String deviceId) => _call(() async {
    final res = await _dio.get('/api/rules', queryParameters: {'device_id': deviceId});
    return (res.data as List).map((e) => SensorRule.fromJson(e as Map<String, dynamic>)).toList();
  });

  Future<SensorRule> createRule(String deviceId, String sensorType,
      String conditionOp, double conditionVal, Map<String, dynamic> command) => _call(() async {
    final res = await _dio.post('/api/rules', data: {
      'device_id': deviceId, 'sensor_type': sensorType,
      'condition_op': conditionOp, 'condition_val': conditionVal,
      'command': command,
    });
    return SensorRule.fromJson(res.data as Map<String, dynamic>);
  });

  Future<void> deleteRule(int id) => _call(() => _dio.delete('/api/rules/$id'));
}
