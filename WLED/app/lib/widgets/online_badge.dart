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
