import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../providers/auth_provider.dart';
import '../providers/mqtt_provider.dart';
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
