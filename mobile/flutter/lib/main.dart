import 'package:flutter/material.dart';

import 'ble/client.dart';
import 'ble/protocol.dart';
import 'cai_runtime.dart';

void main() {
  runApp(const ChipAiApp());
}

class ChipAiApp extends StatelessWidget {
  const ChipAiApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'ChipAI',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: const Color(0xFF1B7F79)),
        useMaterial3: true,
      ),
      home: const HomeScreen(),
    );
  }
}

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  late final CaiRuntime _runtime;
  CaiBleClient? _ble;
  CaiInferResult? _localResult;
  CaiChipResult? _chipResult;
  CaiStatus? _chipStatus;
  bool _inferring = false;
  bool _connecting = false;
  String? _deviceName;
  String? _error;

  @override
  void initState() {
    super.initState();
    _runtime = CaiRuntime(labels: const ['idle', 'walk', 'run']);
    _bootstrap();
  }

  Future<void> _bootstrap() async {
    await _runtime.loadFromAsset('assets/models/demo.tflite');
    if (mounted) setState(() {});
  }

  Future<void> _runLocalInfer() async {
    setState(() => _inferring = true);
    final result = await _runtime.infer(const [0.2, 0.5, 0.1]);
    setState(() {
      _localResult = result;
      _inferring = false;
    });
  }

  Future<void> _connectChip() async {
    setState(() {
      _connecting = true;
      _error = null;
    });

    try {
      _ble?.disconnect();
      _ble = CaiBleClient(_onChipEvent);
      await _ble!.connect();
      setState(() {
        _deviceName = 'ChipAI';
        _connecting = false;
      });
    } catch (e) {
      setState(() {
        _error = e.toString();
        _connecting = false;
      });
    }
  }

  Future<void> _disconnectChip() async {
    await _ble?.disconnect();
    setState(() {
      _ble = null;
      _deviceName = null;
      _chipStatus = null;
      _chipResult = null;
    });
  }

  void _onChipEvent(CaiChipEvent event) {
    setState(() {
      switch (event) {
        case CaiStatusEvent(:final status):
          _chipStatus = status;
        case CaiResultEvent(:final result):
          _chipResult = result;
        case CaiDisconnectedEvent():
          _deviceName = null;
          _chipStatus = null;
        case CaiErrorEvent(:final code):
          _error = 'Chip error 0x${code.toRadixString(16)}';
        case CaiPongEvent():
          break;
      }
    });
  }

  @override
  void dispose() {
    _ble?.disconnect();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final localLabel = _localResult != null && _localResult!.topIndex >= 0
        ? _runtime.labels[_localResult!.topIndex]
        : '-';
    final chipLabel = _chipResult != null && _chipResult!.topIndex < _runtime.labels.length
        ? _runtime.labels[_chipResult!.topIndex]
        : '-';

    return Scaffold(
      appBar: AppBar(title: const Text('ChipAI')),
      body: Padding(
        padding: const EdgeInsets.all(20),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('Local runtime: ${_runtime.isLoaded ? "ready" : "loading"}'),
            Text('Top label: $localLabel'),
            Text('Score: ${_localResult?.topScore.toStringAsFixed(3) ?? '-'}'),
            const SizedBox(height: 16),
            FilledButton(
              onPressed: _inferring ? null : _runLocalInfer,
              child: Text(_inferring ? 'Inferring...' : 'Run local infer'),
            ),
            const Divider(height: 32),
            Text('BLE: ${_deviceName ?? "disconnected"}'),
            if (_ble?.firmwareVersion != null)
              Text('Firmware: ${_ble!.firmwareVersion}'),
            if (_chipStatus != null) ...[
              Text('Chip loaded: ${_chipStatus!.loaded}'),
              Text('Infer: ${_chipStatus!.inferEnabled ? "on" : "off"}'),
              Text('Battery: ${_chipStatus!.batteryPct}%'),
            ],
            Text('Chip top: $chipLabel (${_chipResult?.topScore.toStringAsFixed(3) ?? "-"})'),
            if (_error != null)
              Text(_error!, style: const TextStyle(color: Colors.red)),
            const SizedBox(height: 12),
            Row(
              children: [
                FilledButton.tonal(
                  onPressed: _connecting ? null : (_deviceName == null ? _connectChip : null),
                  child: Text(_connecting ? 'Scanning...' : 'Connect chip'),
                ),
                const SizedBox(width: 8),
                if (_deviceName != null)
                  OutlinedButton(
                    onPressed: _disconnectChip,
                    child: const Text('Disconnect'),
                  ),
              ],
            ),
            if (_deviceName != null) ...[
              const SizedBox(height: 8),
              Wrap(
                spacing: 8,
                children: [
                  OutlinedButton(onPressed: () => _ble?.startInfer(), child: const Text('Start')),
                  OutlinedButton(onPressed: () => _ble?.stopInfer(), child: const Text('Stop')),
                  OutlinedButton(
                    onPressed: () => _ble?.requestStatus(),
                    child: const Text('Status'),
                  ),
                ],
              ),
            ],
            const Spacer(),
            const Text(
              'BLE GATT + TFLite Micro on ESP32/nRF52',
              style: TextStyle(color: Colors.black54),
            ),
          ],
        ),
      ),
    );
  }
}
