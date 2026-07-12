import 'package:flutter_test/flutter_test.dart';
import 'package:chip_ai_mobile/cai_runtime.dart';

void main() {
  test('local infer returns softmax scores', () async {
    final runtime = CaiRuntime(labels: const ['idle', 'walk', 'run']);
    await runtime.loadFromAsset('assets/models/demo.tflite');

    final result = await runtime.infer(const [0.2, 0.5, 0.1]);
    expect(result.topIndex, greaterThanOrEqualTo(0));
    expect(result.scores.length, 3);

    final sum = result.scores.reduce((a, b) => a + b);
    expect(sum, closeTo(1.0, 0.01));
  });
}
