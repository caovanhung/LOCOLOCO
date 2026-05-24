import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../providers/auth_provider.dart';

class AwaitVerifyScreen extends ConsumerStatefulWidget {
  final String email;
  const AwaitVerifyScreen({super.key, required this.email});
  @override
  ConsumerState<AwaitVerifyScreen> createState() => _AwaitVerifyScreenState();
}

class _AwaitVerifyScreenState extends ConsumerState<AwaitVerifyScreen> {
  bool _isSending = false;

  Future<void> _resend() async {
    setState(() { _isSending = true; });
    try {
      final api = ref.read(apiServiceProvider);
      await api.resendVerification(widget.email);
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('Email xác minh đã được gửi lại.')));
      }
    } catch (_) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('Không thể gửi. Thử lại sau.')));
      }
    } finally {
      if (mounted) setState(() { _isSending = false; });
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Kiểm tra email')),
      body: Padding(
        padding: const EdgeInsets.all(24),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            const Icon(Icons.email_outlined, size: 72, color: Colors.blue),
            const SizedBox(height: 24),
            const Text('Xác minh tài khoản',
                style: TextStyle(fontSize: 22, fontWeight: FontWeight.bold)),
            const SizedBox(height: 12),
            Text(
              'Email xác minh đã gửi đến:\n${widget.email}',
              textAlign: TextAlign.center,
              style: const TextStyle(fontSize: 15),
            ),
            const SizedBox(height: 8),
            const Text(
              'Click link trong email để kích hoạt tài khoản.\nLink có hiệu lực trong 24 giờ.',
              textAlign: TextAlign.center,
              style: TextStyle(color: Colors.grey),
            ),
            const SizedBox(height: 32),
            ElevatedButton(
              onPressed: _isSending ? null : _resend,
              child: _isSending
                  ? const SizedBox(width: 20, height: 20,
                      child: CircularProgressIndicator(strokeWidth: 2))
                  : const Text('Gửi lại email'),
            ),
            const SizedBox(height: 12),
            TextButton(
              onPressed: () => Navigator.of(context).popUntil((r) => r.isFirst),
              child: const Text('Về trang Đăng nhập'),
            ),
          ],
        ),
      ),
    );
  }
}
