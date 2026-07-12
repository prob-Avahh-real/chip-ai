import 'dart:async';
import 'dart:typed_data';

import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import 'protocol.dart';

typedef CaiEventHandler = void Function(CaiChipEvent event);

class CaiBleClient {
  CaiBleClient(this.onEvent);

  final CaiEventHandler onEvent;

  BluetoothDevice? _device;
  BluetoothCharacteristic? _controlChar;
  StreamSubscription<List<int>>? _statusSub;
  StreamSubscription<List<int>>? _resultSub;
  Timer? _pingTimer;

  bool connected = false;
  String firmwareVersion = 'unknown';

  Future<void> connect({Duration timeout = const Duration(seconds: 15)}) async {
    final supported = await FlutterBluePlus.isSupported;
    if (!supported) {
      throw StateError('BLE not supported on this device');
    }

    await FlutterBluePlus.startScan(
      timeout: timeout,
      withServices: [Guid(CaiBleProtocol.serviceUuid)],
      withKeywords: [CaiBleProtocol.deviceNamePrefix],
    );

    BluetoothDevice? target;
    final deadline = DateTime.now().add(timeout);
    while (DateTime.now().isBefore(deadline)) {
      for (final result in FlutterBluePlus.lastScanResults) {
        final name = result.device.platformName;
        if (name.startsWith(CaiBleProtocol.deviceNamePrefix)) {
          target = result.device;
          break;
        }
      }
      if (target != null) break;
      await Future<void>.delayed(const Duration(milliseconds: 300));
    }

    await FlutterBluePlus.stopScan();
    if (target == null) {
      throw StateError('No ChipAI device found');
    }

    _device = target;
    await _device!.connect(timeout: timeout, autoConnect: false);

    final services = await _device!.discoverServices();
    final service = services.firstWhere(
      (s) => _uuidEquals(s.uuid, CaiBleProtocol.serviceUuid),
      orElse: () => throw StateError('ChipAI GATT service not found'),
    );

    _controlChar = service.characteristics.firstWhere(
      (c) => _uuidEquals(c.uuid, CaiBleProtocol.controlUuid),
    );

    final statusChar = service.characteristics.firstWhere(
      (c) => _uuidEquals(c.uuid, CaiBleProtocol.statusUuid),
    );
    await statusChar.setNotifyValue(true);
    _statusSub = statusChar.onValueReceived.listen(_onNotify);

    final resultChar = service.characteristics.firstWhere(
      (c) => _uuidEquals(c.uuid, CaiBleProtocol.resultUuid),
    );
    await resultChar.setNotifyValue(true);
    _resultSub = resultChar.onValueReceived.listen(_onNotify);

    try {
      final infoChar = service.characteristics.firstWhere(
        (c) => _uuidEquals(c.uuid, CaiBleProtocol.deviceInfoUuid),
      );
      final info = await infoChar.read();
      firmwareVersion = String.fromCharCodes(info);
    } catch (_) {
      firmwareVersion = 'unknown';
    }

    await write(CaiBleProtocol.cmdRequestStatus());
    connected = true;
    _startPing();
  }

  Future<void> disconnect() async {
    _stopPing();
    await _statusSub?.cancel();
    await _resultSub?.cancel();
    _statusSub = null;
    _resultSub = null;
    _controlChar = null;

    if (_device != null) {
      await _device!.disconnect();
      _device = null;
    }

    connected = false;
    onEvent(const CaiChipEvent.disconnected());
  }

  Future<void> requestStatus() => write(CaiBleProtocol.cmdRequestStatus());
  Future<void> startInfer() => write(CaiBleProtocol.cmdStartInfer());
  Future<void> stopInfer() => write(CaiBleProtocol.cmdStopInfer());
  Future<void> setThreshold(double value) => write(CaiBleProtocol.cmdSetThreshold(value));

  Future<void> write(List<int> data) async {
    if (_controlChar == null) {
      throw StateError('ChipAI not connected');
    }
    await _controlChar!.write(data, withoutResponse: true);
  }

  void _onNotify(List<int> data) {
    final frame = CaiBleProtocol.decodeFrame(Uint8List.fromList(data));
    if (frame == null) return;
    final event = CaiBleProtocol.parseEvent(frame);
    if (event != null) onEvent(event);
  }

  void _startPing() {
    _stopPing();
    _pingTimer = Timer.periodic(const Duration(seconds: 30), (_) {
      if (connected) {
        write(CaiBleProtocol.cmdPing()).catchError((_) {});
      }
    });
  }

  void _stopPing() {
    _pingTimer?.cancel();
    _pingTimer = null;
  }

  bool _uuidEquals(Guid guid, String expected) {
    return guid.str.toLowerCase() == expected.toLowerCase();
  }
}
