import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../services/api_service.dart';

const _serverUrl = 'https://hung-test.codeaplha.biz';

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
