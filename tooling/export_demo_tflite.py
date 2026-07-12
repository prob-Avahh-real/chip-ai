#!/usr/bin/env python3
"""Export a tiny 3-class demo TFLite model for ChipAI firmware."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


def export_with_tensorflow(out_tflite: Path, out_c: Path) -> None:
    import numpy as np
    import tensorflow as tf

    model = tf.keras.Sequential(
        [
            tf.keras.layers.Input(shape=(3,), name="features"),
            tf.keras.layers.Dense(3, activation="softmax", name="probs"),
        ]
    )
    model.compile(optimizer="adam", loss="sparse_categorical_crossentropy")

    # Bake deterministic weights close to stub backend behaviour.
    dense = model.layers[1]
    dense.set_weights(
        [
            np.array([[1.0, 0.5, -0.2], [0.8, 0.4, -0.1], [0.2, 0.1, 0.3]], dtype=np.float32),
            np.array([0.1, 0.2, 0.0], dtype=np.float32),
        ]
    )

    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = []
    tflite_model = converter.convert()

    out_tflite.parent.mkdir(parents=True, exist_ok=True)
    out_tflite.write_bytes(tflite_model)
    _write_c_array(out_c, tflite_model, "g_demo_tflite_model")
    print(f"wrote {out_tflite} ({len(tflite_model)} bytes)")
    print(f"wrote {out_c}")


def export_stub_only(out_tflite: Path, out_c: Path) -> None:
    """Fallback when TensorFlow is unavailable: export stub weights blob."""
    blob = struct.pack("<3f", 1.0, 0.5, -0.2) + struct.pack("<f", 0.1)
    out_tflite.parent.mkdir(parents=True, exist_ok=True)
    out_tflite.write_bytes(blob)
    _write_c_array(out_c, blob, "g_demo_tflite_model")
    print(f"tensorflow unavailable; wrote stub blob {out_tflite}")
    print(f"wrote {out_c}")


def _write_c_array(path: Path, data: bytes, symbol: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    lines = [
        "#include <stdint.h>",
        "",
        f"const uint8_t {symbol}[] = {{",
    ]
    for i in range(0, len(data), 12):
        chunk = data[i : i + 12]
        hexes = ", ".join(f"0x{b:02x}" for b in chunk)
        lines.append(f"    {hexes},")
    lines.extend(
        [
            "};",
            f"const unsigned int {symbol}_len = {len(data)};",
            "",
        ]
    )
    path.write_text("\n".join(lines), encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description="Export ChipAI demo TFLite model")
    parser.add_argument("--out-tflite", type=Path, default=Path("models/demo/demo.tflite"))
    parser.add_argument("--out-c", type=Path, default=Path("firmware/esp32/main/model_demo.c"))
    args = parser.parse_args()

    try:
        export_with_tensorflow(args.out_tflite, args.out_c)
    except ImportError:
        export_stub_only(args.out_tflite.with_suffix(".bin"), args.out_c)
    except Exception as exc:
        print(f"tensorflow export failed ({exc}); falling back to stub blob")
        export_stub_only(args.out_tflite.with_suffix(".bin"), args.out_c)


if __name__ == "__main__":
    main()
