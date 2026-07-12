# ChipAI — 通用于芯片的边缘 AI 项目

跨芯片、跨平台的边缘推理脚手架：**移动端（Flutter/TFLite）** + **智能硬件（ESP32 / nRF52 / TFLite Micro）**，共享 C 运行时与 BLE GATT 协议。

## 架构

```
chip-ai/
├── core/                 # 纯 C 推理运行时（backend ops 抽象）
├── firmware/
│   ├── shared/           # BLE 帧协议 + 应用状态机
│   ├── esp32/            # ESP-IDF + NimBLE GATT + TFLite Micro
│   └── nrf52/            # nRF Connect SDK + Zephyr GATT
├── mobile/flutter/       # iOS/Android BLE 客户端
├── tooling/              # 模型导出
└── models/               # 模型与部署清单
```

## BLE GATT 服务（已实现）

| 特征 | UUID | 属性 |
|------|------|------|
| Service | `ca10a100-0001-4000-8000-000000ca1001` | Primary |
| Control | `ca10a103-...` | Write / Write No Resp |
| Status | `ca10a101-...` | Read / Notify |
| Result | `ca10a102-...` | Notify |
| Device Info | `ca10a104-...` | Read |

帧格式：`SOF('CA') + VER + CMD + LEN + PAYLOAD + CRC8`

广播名：`ChipAI-XXXX`（MAC 后两字节）

### 控制命令（App → 芯片）

| CMD | 值 | 说明 |
|-----|-----|------|
| REQUEST_STATUS | 0x01 | 立即上报状态 |
| START_INFER | 0x02 | 开启周期推理 |
| STOP_INFER | 0x03 | 停止推理 |
| SET_THRESHOLD | 0x04 | payload: float32 LE |
| PING | 0x05 | 心跳 |

### 事件（芯片 → App）

| EVT | 值 | 通道 |
|-----|-----|------|
| STATUS | 0x80 | Status notify |
| RESULT | 0x81 | Result notify |
| PONG | 0x85 | Status notify |
| ERROR | 0x8F | Status notify |

## 快速开始

```bash
# 协议 + 核心运行时测试
npm test

# 导出 demo 模型（有 TensorFlow 时生成真实 .tflite）
npm run export:demo

# ESP32（含 esp-tflite-micro 时自动启用 CAI_HAS_TFLITE_MICRO）
cd firmware/esp32
idf.py set-target esp32
idf.py build flash monitor

# nRF52
cd firmware/nrf52
west build -b nrf52840dk_nrf52840
west flash

# Flutter 移动端
cd mobile/flutter
flutter pub get
flutter run
```

## 推理后端

| 后端 | 芯片 | 状态 |
|------|------|------|
| `cai_stub_backend` | 全平台 / CI | ✅ |
| `cai_tflite_micro_backend` | ESP32（esp-tflite-micro） | ✅ |
| TFLite Flutter | iOS/Android | 🔌 骨架 |
| ONNX | Linux 边缘 | 📋 预留 |

ESP32 构建时若检测到 `espressif/esp-tflite-micro` 组件，自动定义 `CAI_HAS_TFLITE_MICRO` 并加载 `model_demo.c` 内嵌模型。

## 与手机联调

1. 烧录 ESP32/nRF52 固件
2. 手机 App 点 **Connect chip**（扫描 `ChipAI-*`）
3. 自动订阅 Status / Result 通知
4. 使用 Start / Stop / Status 按钮验证双向通信
5. Result 通知含 `top_index` + `top_score` + `infer_us`

## 环境要求

- C11 编译器
- ESP-IDF v5.x + NimBLE
- nRF Connect SDK v2.x
- Flutter 3.3+ / `flutter_blue_plus`
- Python 3.10+（可选 TensorFlow 导出真实 demo 模型）
