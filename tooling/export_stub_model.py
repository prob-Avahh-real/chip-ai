#!/usr/bin/env python3
"""Export a tiny stub model blob compatible with core/src/cai_stub_backend.c."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


def export_stub(weights: list[float], bias: float, out: Path) -> None:
    if len(weights) != 3:
        raise ValueError("stub backend expects exactly 3 input weights")
    blob = b"".join(struct.pack("<f", w) for w in weights) + struct.pack("<f", bias)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(blob)
    print(f"wrote {out} ({len(blob)} bytes)")


def main() -> None:
    parser = argparse.ArgumentParser(description="Export ChipAI stub model blob")
    parser.add_argument("--weights", nargs=3, type=float, default=[1.0, 0.5, -0.2])
    parser.add_argument("--bias", type=float, default=0.1)
    parser.add_argument("--out", type=Path, default=Path("models/stub/model.bin"))
    args = parser.parse_args()
    export_stub(args.weights, args.bias, args.out)


if __name__ == "__main__":
    main()
