import 'dart:typed_data';

/// ChipAI BLE protocol v1 — mirrors firmware/shared/cai_protocol.h
class CaiBleProtocol {
  static const int protocolVersion = 1;
  static const int sof0 = 0x43; // 'C'
  static const int sof1 = 0x41; // 'A'
  static const int maxPayload = 32;

  static const String serviceUuid = 'ca10a100-0001-4000-8000-000000ca1001';
  static const String controlUuid = 'ca10a103-0001-4000-8000-000000ca1001';
  static const String statusUuid = 'ca10a101-0001-4000-8000-000000ca1001';
  static const String resultUuid = 'ca10a102-0001-4000-8000-000000ca1001';
  static const String deviceInfoUuid = 'ca10a104-0001-4000-8000-000000ca1001';
  static const String deviceNamePrefix = 'ChipAI';

  static const int cmdRequestStatus = 0x01;
  static const int cmdStartInfer = 0x02;
  static const int cmdStopInfer = 0x03;
  static const int cmdSetThreshold = 0x04;
  static const int cmdPing = 0x05;

  static const int evtStatus = 0x80;
  static const int evtResult = 0x81;
  static const int evtPong = 0x85;
  static const int evtError = 0x8f;

  static const int flagConnected = 1 << 0;
  static const int flagInferEnabled = 1 << 1;

  static int crc8(List<int> data) {
    var crc = 0;
    for (final b in data) {
      crc ^= b;
      for (var i = 0; i < 8; i++) {
        crc = (crc & 0x80) != 0 ? ((crc << 1) ^ 0x07) & 0xff : (crc << 1) & 0xff;
      }
    }
    return crc;
  }

  static Uint8List encodeFrame(int cmd, [List<int> payload = const []]) {
    final body = <int>[protocolVersion, cmd, payload.length, ...payload];
    final frame = Uint8List(2 + body.length + 1);
    frame[0] = sof0;
    frame[1] = sof1;
    frame.setRange(2, 2 + body.length, body);
    frame[frame.length - 1] = crc8(body);
    return frame;
  }

  static CaiFrame? decodeFrame(Uint8List data) {
    if (data.length < 6 || data[0] != sof0 || data[1] != sof1) return null;
    if (data[2] != protocolVersion) return null;

    final cmd = data[3];
    final len = data[4];
    if (data.length < 5 + len + 1 || len > maxPayload) return null;

    final body = data.sublist(2, 5 + len);
    if (crc8(body) != data[5 + len]) return null;

    return CaiFrame(cmd: cmd, payload: data.sublist(5, 5 + len));
  }

  static Uint8List cmdRequestStatus() => encodeFrame(cmdRequestStatus);
  static Uint8List cmdStartInfer() => encodeFrame(cmdStartInfer);
  static Uint8List cmdStopInfer() => encodeFrame(cmdStopInfer);
  static Uint8List cmdPing() => encodeFrame(cmdPing);

  static Uint8List cmdSetThreshold(double value) {
    final bytes = ByteData(4);
    bytes.setFloat32(0, value, Endian.little);
    return encodeFrame(cmdSetThreshold, bytes.buffer.asUint8List().toList());
  }

  static CaiStatus? parseStatusPayload(Uint8List payload) {
    if (payload.length < 14) return null;
    final inferCount = ByteData.sublistView(payload, 5, 9).getUint32(0, Endian.little);
    final arenaKb = payload[9] | (payload[10] << 8);
    final threshold = ByteData.sublistView(payload, 11, 15).getFloat32(0, Endian.little);
    return CaiStatus(
      loaded: payload[0] == 1,
      backendKind: payload[1],
      labelCount: payload[2],
      batteryPct: payload[3],
      flags: payload[4],
      inferCount: inferCount,
      arenaKb: arenaKb,
      threshold: threshold,
    );
  }

  static CaiChipResult? parseResultPayload(Uint8List payload) {
    if (payload.length < 8) return null;
    final inferUs = payload[2] | (payload[3] << 8);
    final topScore = ByteData.sublistView(payload, 4, 8).getFloat32(0, Endian.little);
    return CaiChipResult(
      topIndex: payload[0],
      scoreCount: payload[1],
      inferUs: inferUs,
      topScore: topScore,
    );
  }

  static CaiChipEvent? parseEvent(CaiFrame frame) {
    switch (frame.cmd) {
      case evtStatus:
        final status = parseStatusPayload(frame.payload);
        return status == null ? null : CaiChipEvent.status(status);
      case evtResult:
        final result = parseResultPayload(frame.payload);
        return result == null ? null : CaiChipEvent.result(result);
      case evtPong:
        return const CaiChipEvent.pong();
      case evtError:
        return CaiChipEvent.error(frame.payload.isNotEmpty ? frame.payload[0] : 0xff);
      default:
        return null;
    }
  }
}

class CaiFrame {
  final int cmd;
  final Uint8List payload;

  const CaiFrame({required this.cmd, required this.payload});
}

class CaiStatus {
  final bool loaded;
  final int backendKind;
  final int labelCount;
  final int batteryPct;
  final int flags;
  final int inferCount;
  final int arenaKb;
  final double threshold;

  const CaiStatus({
    required this.loaded,
    required this.backendKind,
    required this.labelCount,
    required this.batteryPct,
    required this.flags,
    required this.inferCount,
    required this.arenaKb,
    required this.threshold,
  });

  bool get connected => (flags & CaiBleProtocol.flagConnected) != 0;
  bool get inferEnabled => (flags & CaiBleProtocol.flagInferEnabled) != 0;
}

class CaiChipResult {
  final int topIndex;
  final int scoreCount;
  final int inferUs;
  final double topScore;

  const CaiChipResult({
    required this.topIndex,
    required this.scoreCount,
    required this.inferUs,
    required this.topScore,
  });
}

sealed class CaiChipEvent {
  const CaiChipEvent();

  const factory CaiChipEvent.status(CaiStatus status) = CaiStatusEvent;
  const factory CaiChipEvent.result(CaiChipResult result) = CaiResultEvent;
  const factory CaiChipEvent.pong() = CaiPongEvent;
  const factory CaiChipEvent.error(int code) = CaiErrorEvent;
  const factory CaiChipEvent.disconnected() = CaiDisconnectedEvent;
}

class CaiStatusEvent extends CaiChipEvent {
  final CaiStatus status;
  const CaiStatusEvent(this.status);
}

class CaiResultEvent extends CaiChipEvent {
  final CaiChipResult result;
  const CaiResultEvent(this.result);
}

class CaiPongEvent extends CaiChipEvent {
  const CaiPongEvent();
}

class CaiErrorEvent extends CaiChipEvent {
  final int code;
  const CaiErrorEvent(this.code);
}

class CaiDisconnectedEvent extends CaiChipEvent {
  const CaiDisconnectedEvent();
}
