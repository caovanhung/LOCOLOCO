import 'package:flutter_test/flutter_test.dart';
import 'package:locoloco/models/mqtt_message.dart';
import 'package:locoloco/models/device.dart';

void main() {
  group('MqttEnvelope.tryParse', () {
    test('parses valid envelope', () {
      const raw = '{"v":1,"msgId":"abc","ts":1000,"devId":"esp_001","type":"rpt","data":{"dps":{"1":true,"2":180,"3":1,"4":"255,0,0"}}}';
      final env = MqttEnvelope.tryParse(raw);
      expect(env, isNotNull);
      expect(env!.devId, 'esp_001');
      expect(env.type, 'rpt');
      expect(env.data['dps']['1'], true);
    });

    test('returns null for invalid JSON', () {
      expect(MqttEnvelope.tryParse('not json'), isNull);
    });
  });

  group('LedState.fromDps', () {
    test('parses DPs correctly', () {
      final state = LedState.fromDps({'1': true, '2': 180, '3': 2, '4': '255,100,0'});
      expect(state.on, true);
      expect(state.brightness, 180);
      expect(state.r, 255);
      expect(state.g, 100);
      expect(state.b, 0);
    });

    test('handles missing DPs with defaults', () {
      final state = LedState.fromDps({});
      expect(state.on, false);
      expect(state.brightness, 200);
    });
  });
}
