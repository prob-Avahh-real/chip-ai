import 'dart:typed_data';

import 'package:flutter_test/flutter_test.dart';
import 'package:chip_ai_mobile/ble/protocol.dart';

void main() {
  test('encode/decode roundtrip', () {
    final frame = CaiBleProtocol.encodeFrame(CaiBleProtocol.cmdStartInfer);
    final decoded = CaiBleProtocol.decodeFrame(frame);
    expect(decoded, isNotNull);
    expect(decoded!.cmd, CaiBleProtocol.cmdStartInfer);
    expect(decoded.payload, isEmpty);
  });

  test('parse status payload', () {
    final payload = Uint8List(14);
    payload[0] = 1;
    payload[1] = 2;
    payload[2] = 3;
    payload[3] = 88;
    payload[4] = CaiBleProtocol.flagConnected | CaiBleProtocol.flagInferEnabled;
    ByteData.sublistView(payload, 5, 9).setUint32(0, 42, Endian.little);
    payload[9] = 8;
    payload[10] = 0;
    ByteData.sublistView(payload, 11, 15).setFloat32(0, 0.6, Endian.little);

    final status = CaiBleProtocol.parseStatusPayload(payload);
    expect(status, isNotNull);
    expect(status!.loaded, isTrue);
    expect(status.inferCount, 42);
    expect(status.connected, isTrue);
    expect(status.inferEnabled, isTrue);
    expect(status.threshold, closeTo(0.6, 0.001));
  });
}
