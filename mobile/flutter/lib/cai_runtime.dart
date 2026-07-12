import 'dart:typed_data';

/// Cross-platform inference facade.
/// Mobile uses TFLite; firmware uses TFLite Micro / stub via shared C runtime.
class CaiRuntime {
  bool _loaded = false;
  final List<String> labels;
  final double threshold;

  CaiRuntime({
    required this.labels,
    this.threshold = 0.5,
  });

  bool get isLoaded => _loaded;

  Future<void> loadFromAsset(String modelPath) async {
    // TFLite asset loading via tflite_flutter lands here.
    // Stub path keeps tests runnable without a bundled model file.
    _loaded = true;
  }

  Future<CaiInferResult> infer(List<double> features) async {
    if (!_loaded || features.length < 3) {
      return const CaiInferResult(topIndex: -1, topScore: 0, scores: []);
    }

    final logits = List<double>.generate(labels.length, (i) {
      final bias = 0.1 + i * 0.1;
      return bias + features[0] + features[1] * 0.5 - features[2] * 0.2;
    });

    final maxLogit = logits.reduce((a, b) => a > b ? a : b);
    final expScores = logits.map((v) => _exp(v - maxLogit)).toList();
    final sum = expScores.reduce((a, b) => a + b);
    final scores = expScores.map((v) => v / sum).toList();

    var topIndex = 0;
    var topScore = scores[0];
    for (var i = 1; i < scores.length; i++) {
      if (scores[i] > topScore) {
        topScore = scores[i];
        topIndex = i;
      }
    }

    return CaiInferResult(
      topIndex: topIndex,
      topScore: topScore,
      scores: scores,
    );
  }

  double _exp(double x) {
    var term = 1.0;
    var sum = 1.0;
    for (var i = 1; i < 8; i++) {
      term *= x / i;
      sum += term;
    }
    return sum;
  }
}

class CaiInferResult {
  final int topIndex;
  final double topScore;
  final List<double> scores;

  const CaiInferResult({
    required this.topIndex,
    required this.topScore,
    required this.scores,
  });

  bool get aboveThreshold => topScore >= 0.5;
}
