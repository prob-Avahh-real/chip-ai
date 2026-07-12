#!/usr/bin/env python3
"""Convert ONNX classifier to deployment manifests for mobile + embedded targets."""

from __future__ import annotations

import argparse
import json
from pathlib import Path


def write_manifest(model_path: Path, labels: list[str], out_dir: Path) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)

    manifest = {
        "name": model_path.stem,
        "source": str(model_path),
        "labels": labels,
        "targets": {
            "mobile": {
                "format": "tflite",
                "path": f"assets/models/{model_path.stem}.tflite",
            },
            "esp32": {
                "format": "tflite_micro",
                "path": f"firmware/esp32/models/{model_path.stem}.tflite",
            },
            "nrf52": {
                "format": "tflite_micro",
                "path": f"firmware/nrf52/models/{model_path.stem}.tflite",
            },
        },
        "notes": "Run onnx2tf / tensorflow lite converter in your training pipeline.",
    }

    manifest_path = out_dir / f"{model_path.stem}.manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(f"wrote {manifest_path}")


def main() -> None:
    parser = argparse.ArgumentParser(description="ChipAI model conversion manifest")
    parser.add_argument("model", type=Path, help="Path to ONNX model")
    parser.add_argument("--labels", nargs="+", default=["idle", "walk", "run"])
    parser.add_argument("--out", type=Path, default=Path("models/manifests"))
    args = parser.parse_args()

    if not args.model.exists():
        raise SystemExit(f"model not found: {args.model}")

    write_manifest(args.model, args.labels, args.out)


if __name__ == "__main__":
    main()
