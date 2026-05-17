// app/lib/widgets/led_control_panel.dart
import 'package:flutter/material.dart';
import 'package:flex_color_picker/flex_color_picker.dart';
import '../models/device.dart';

const effectNames = ['OFF', 'SOLID', 'BLINK', 'FADE', 'CHASE'];

class LedControlPanel extends StatelessWidget {
  final LedState state;
  final void Function(LedState) onChanged;

  const LedControlPanel({super.key, required this.state, required this.onChanged});

  @override
  Widget build(BuildContext context) {
    return Column(children: [
      SwitchListTile(
        title: const Text('Bật/Tắt'),
        value: state.on,
        onChanged: (v) => onChanged(state.copyWith(on: v)),
      ),
      ListTile(
        title: const Text('Hiệu ứng'),
        trailing: DropdownButton<int>(
          value: state.effect.clamp(0, 4),
          items: List.generate(5, (i) =>
              DropdownMenuItem(value: i, child: Text(effectNames[i]))),
          onChanged: (v) { if (v != null) onChanged(state.copyWith(effect: v)); },
        ),
      ),
      ListTile(
        title: Text('Độ sáng: ${state.brightness}'),
        subtitle: Slider(
          value: state.brightness.toDouble(),
          min: 0, max: 255,
          onChanged: (v) => onChanged(state.copyWith(brightness: v.toInt())),
        ),
      ),
      ListTile(
        title: const Text('Màu sắc'),
        trailing: ColorIndicator(
          width: 44, height: 44, borderRadius: 22,
          color: Color.fromRGBO(state.r, state.g, state.b, 1),
          onSelectFocus: false,
          onSelect: () async {
            Color picked = Color.fromRGBO(state.r, state.g, state.b, 1);
            final ok = await ColorPicker(
              color: picked,
              onColorChanged: (c) => picked = c,
              title: const Text('Chọn màu'),
              pickersEnabled: const {ColorPickerType.wheel: true},
            ).showPickerDialog(context);
            if (ok) {
              onChanged(state.copyWith(r: picked.r.toInt(), g: picked.g.toInt(), b: picked.b.toInt()));
            }
          },
        ),
      ),
    ]);
  }
}
