import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../models/device.dart';
import 'auth_provider.dart';

final deviceListProvider = FutureProvider<List<Device>>((ref) async {
  final api = ref.watch(apiServiceProvider);
  return api.getDevices();
});

final selectedDeviceStateProvider = StateProvider.family<LedState?, String>((ref, deviceId) => null);
final deviceOnlineProvider = StateProvider.family<bool, String>((ref, deviceId) => false);
