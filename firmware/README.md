# ChipAI 固件

ESP32 与 nRF52 共用 `shared/` 协议层、`core/` C 运行时，以及完整 BLE GATT 服务。

## 目录

```
firmware/
├── shared/          # 帧协议 (cai_protocol) + 应用状态机 (cai_app)
├── esp32/           # ESP-IDF + NimBLE + TFLite Micro（可选）
└── nrf52/           # nRF Connect SDK + Zephyr Bluetooth
```

## GATT 特征

与 `mobile/flutter/lib/ble/protocol.dart` 完全一致：

- **Control**：写入帧化命令（REQUEST_STATUS / START_INFER / STOP_INFER / SET_THRESHOLD / PING）
- **Status**：Read 返回当前状态帧；Notify 推送 STATUS / PONG / ERROR
- **Result**：Notify 推送推理 RESULT
- **Device Info**：固件版本字符串

## ESP32

```bash
cd firmware/esp32
idf.py set-target esp32
idf.py build flash monitor
```

依赖：
- ESP-IDF v5.x（NimBLE 已默认启用）
- `idf_component.yml` 拉取 `espressif/esp-tflite-micro`

启用 TFLite Micro 后，`main.c` 自动切换 `cai_tflite_micro_backend()`；否则回退 stub 后端。

## nRF52

```bash
cd firmware/nrf52
west build -b nrf52840dk_nrf52840
west flash
```

`prj.conf` 已启用 `CONFIG_BT_PERIPHERAL`。

## 推理周期

- Status 周期上报：2s
- 推理周期：200ms（`infer_enabled` 时）
- LED：`top_score >= threshold` 时点亮

## 联调检查清单

- [ ] 广播名 `ChipAI-XXXX` 可见
- [ ] 连接后 Status notify 收到 14 字节 payload
- [ ] `START_INFER` 后 Result notify 周期到达
- [ ] `SET_THRESHOLD` 后 status 中 threshold 字段更新
- [ ] `PING` 收到 PONG
