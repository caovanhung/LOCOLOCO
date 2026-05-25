import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:shared_preferences/shared_preferences.dart';
import '../services/api_service.dart';

const _serverUrl = 'https://hung-test.codeaplha.biz';
const _kToken = 'auth_token';
const _kMqttHost = 'mqtt_host';
const _kMqttPort = 'mqtt_port';
const _kMqttUser = 'mqtt_user';
const _kMqttPass = 'mqtt_pass';

final apiServiceProvider = Provider<ApiService>((ref) => ApiService(_serverUrl));

class AuthState {
  final bool isLoading;
  final String? token;
  final MqttCredentials? mqttCreds;
  final String? error;
  final bool isUnverified;

  const AuthState({
    this.isLoading = false,
    this.token,
    this.mqttCreds,
    this.error,
    this.isUnverified = false,
  });

  bool get isLoggedIn => token != null;

  AuthState copyWith({
    bool? isLoading,
    String? token,
    MqttCredentials? mqttCreds,
    String? error,
    bool? isUnverified,
  }) => AuthState(
    isLoading: isLoading ?? this.isLoading,
    token: token ?? this.token,
    mqttCreds: mqttCreds ?? this.mqttCreds,
    error: error,
    isUnverified: isUnverified ?? false,
  );
}

class AuthNotifier extends StateNotifier<AuthState> {
  final ApiService _api;
  AuthNotifier(this._api) : super(const AuthState());

  Future<void> tryRestoreSession() async {
    final prefs = await SharedPreferences.getInstance();
    final token = prefs.getString(_kToken);
    final host  = prefs.getString(_kMqttHost);
    final port  = prefs.getInt(_kMqttPort);
    final user  = prefs.getString(_kMqttUser);
    final pass  = prefs.getString(_kMqttPass);
    if (token != null && host != null && port != null && user != null && pass != null) {
      _api.setToken(token);
      state = state.copyWith(
        token: token,
        mqttCreds: MqttCredentials(host: host, port: port, username: user, password: pass),
      );
    }
  }

  Future<void> login(String username, String password) async {
    state = state.copyWith(isLoading: true, error: null, isUnverified: false);
    try {
      final result = await _api.login(username, password);
      _api.setToken(result.token);
      final prefs = await SharedPreferences.getInstance();
      await prefs.setString(_kToken, result.token);
      await prefs.setString(_kMqttHost, result.mqtt.host);
      await prefs.setInt(_kMqttPort, result.mqtt.port);
      await prefs.setString(_kMqttUser, result.mqtt.username);
      await prefs.setString(_kMqttPass, result.mqtt.password);
      state = state.copyWith(isLoading: false, token: result.token, mqttCreds: result.mqtt);
    } on ApiException catch (e) {
      state = state.copyWith(
        isLoading: false,
        error: e.message,
        isUnverified: e.statusCode == 403,
      );
    } catch (e) {
      state = state.copyWith(isLoading: false, error: e.toString());
    }
  }

  Future<void> logout() async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.clear();
    state = const AuthState();
  }
}

final authProvider = StateNotifierProvider<AuthNotifier, AuthState>((ref) {
  return AuthNotifier(ref.watch(apiServiceProvider));
});
