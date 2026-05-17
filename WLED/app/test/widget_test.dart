import 'package:flutter_test/flutter_test.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:locoloco/main.dart';

void main() {
  testWidgets('App smoke test - renders login screen', (WidgetTester tester) async {
    await tester.pumpWidget(const ProviderScope(child: LocoApp()));
    expect(find.text('LOCOLOCO'), findsOneWidget);
  });
}
