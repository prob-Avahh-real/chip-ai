# 模型目录

## stub 模型（16 字节）

与 `core/src/cai_stub_backend.c` 对齐：

- `weights[3]`：3 × float32
- `bias`：1 × float32

生成：

```bash
python3 tooling/export_stub_model.py --out models/stub/model.bin
```

## 部署清单

`tooling/convert_model.py` 会为每个 ONNX 模型生成 manifest，指明：

- 手机端 TFLite 路径
- ESP32 / nRF52 TFLite Micro 路径
- 标签列表

## 量化建议

| 目标 | 推荐 |
|------|------|
| MCU (ESP32/nRF52) | int8 全量化 TFLite Micro |
| 手机 | float16 或 int8 TFLite |
| 服务器/边缘 Linux | ONNX FP32 / INT8 |
