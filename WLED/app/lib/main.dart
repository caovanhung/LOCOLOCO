import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'providers/auth_provider.dart';
import 'providers/mqtt_provider.dart';
import 'screens/login_screen.dart';
import 'screens/device_list_screen.dart';

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
      home: const _SplashRouter(),
    );
  }
}

class _SplashRouter extends ConsumerStatefulWidget {
  const _SplashRouter();
  @override
  ConsumerState<_SplashRouter> createState() => _SplashRouterState();
}

class _SplashRouterState extends ConsumerState<_SplashRouter> {
  bool _checked = false;

  @override
  void initState() {
    super.initState();
    _tryAutoLogin();
  }

  Future<void> _tryAutoLogin() async {
    await ref.read(authProvider.notifier).tryRestoreSession();
    final auth = ref.read(authProvider);
    if (auth.isLoggedIn && auth.mqttCreds != null) {
      try {
        final mqtt = ref.read(mqttServiceProvider);
        await mqtt.connect(
          host: auth.mqttCreds!.host,
          port: auth.mqttCreds!.port,
          username: auth.mqttCreds!.username,
          password: auth.mqttCreds!.password,
        );
        ref.read(mqttConnectedProvider.notifier).state = mqtt.isConnected;
      } catch (_) {}
    }
    if (mounted) setState(() => _checked = true);
  }

  @override
  Widget build(BuildContext context) {
    if (!_checked) {
      return const Scaffold(body: Center(child: CircularProgressIndicator()));
    }
    final auth = ref.watch(authProvider);
    return auth.isLoggedIn ? const DeviceListScreen() : const LoginScreen();
  }
}
