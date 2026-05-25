import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../providers/auth_provider.dart';
import '../providers/mqtt_provider.dart';
import 'device_list_screen.dart';
import 'register_screen.dart';

class LoginScreen extends ConsumerStatefulWidget {
  const LoginScreen({super.key});
  @override
  ConsumerState<LoginScreen> createState() => _LoginScreenState();
}

class _LoginScreenState extends ConsumerState<LoginScreen> {
  final _userCtrl = TextEditingController();
  final _passCtrl = TextEditingController();

  Future<void> _login() async {
    debugPrint('[LOGIN] starting login');
    await ref.read(authProvider.notifier).login(_userCtrl.text, _passCtrl.text);
    final auth = ref.read(authProvider);
    debugPrint('[LOGIN] isLoggedIn=${auth.isLoggedIn} error=${auth.error}');

    if (!mounted) return;

    if (auth.isUnverified) {
      _showResendDialog();
      return;
    }

    if (auth.isLoggedIn && auth.mqttCreds != null) {
      final mqtt = ref.read(mqttServiceProvider);
      try {
        debugPrint('[LOGIN] connecting MQTT to ${auth.mqttCreds!.host}:${auth.mqttCreds!.port}');
        await mqtt.connect(
          host: auth.mqttCreds!.host,
          port: auth.mqttCreds!.port,
          username: auth.mqttCreds!.username,
          password: auth.mqttCreds!.password,
        );
        debugPrint('[LOGIN] MQTT connected=${mqtt.isConnected}');
      } catch (e) {
        debugPrint('[LOGIN] MQTT error: $e');
      }
      ref.read(mqttConnectedProvider.notifier).state = mqtt.isConnected;
      if (mounted) {
        Navigator.of(context).pushReplacement(
          MaterialPageRoute(builder: (_) => const DeviceListScreen()));
      }
    }
  }

  void _showResendDialog() {
    final emailCtrl = TextEditingController();
    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('Email chưa xác minh'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            const Text('Nhập email để nhận lại link xác minh:'),
            const SizedBox(height: 12),
            TextField(
              controller: emailCtrl,
              keyboardType: TextInputType.emailAddress,
              decoration: const InputDecoration(
                  labelText: 'Email', border: OutlineInputBorder()),
            ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx),
            child: const Text('Hủy'),
          ),
          TextButton(
            onPressed: () async {
              Navigator.pop(ctx);
              try {
                final api = ref.read(apiServiceProvider);
                await api.resendVerification(emailCtrl.text.trim());
                if (mounted) {
                  ScaffoldMessenger.of(context).showSnackBar(
                    const SnackBar(content: Text('Email xác minh đã được gửi lại.')));
                }
              } catch (_) {
                if (mounted) {
                  ScaffoldMessenger.of(context).showSnackBar(
                    const SnackBar(content: Text('Không thể gửi. Thử lại sau.')));
                }
              }
            },
            child: const Text('Gửi lại'),
          ),
        ],
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    final auth = ref.watch(authProvider);
    return Scaffold(
      appBar: AppBar(title: const Text('LOCOLOCO')),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(24),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            const SizedBox(height: 60),
            TextField(
              controller: _userCtrl,
              decoration: const InputDecoration(labelText: 'Username'),
            ),
            const SizedBox(height: 12),
            TextField(
              controller: _passCtrl,
              obscureText: true,
              decoration: const InputDecoration(labelText: 'Password'),
            ),
            const SizedBox(height: 24),
            if (auth.error != null && !auth.isUnverified)
              Text(auth.error!, style: const TextStyle(color: Colors.red)),
            ElevatedButton(
              onPressed: auth.isLoading ? null : _login,
              child: auth.isLoading
                  ? const SizedBox(width: 20, height: 20,
                      child: CircularProgressIndicator(strokeWidth: 2))
                  : const Text('Đăng nhập'),
            ),
            const SizedBox(height: 12),
            TextButton(
              onPressed: () => Navigator.of(context).push(
                MaterialPageRoute(builder: (_) => const RegisterScreen())),
              child: const Text('Chưa có tài khoản? Đăng ký'),
            ),
            const SizedBox(height: 24),
          ],
        ),
      ),
    );
  }
}
