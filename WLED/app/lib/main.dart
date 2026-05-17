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
