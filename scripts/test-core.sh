#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/build"
mkdir -p "$OUT"

cc -std=c11 -Wall -Wextra -I"$ROOT/core/include" \
  "$ROOT/core/src/cai_tensor.c" \
  "$ROOT/core/src/cai_stub_backend.c" \
  "$ROOT/core/src/cai_runtime.c" \
  "$ROOT/core/tests/test_runtime.c" \
  -lm -o "$OUT/test_runtime"

"$OUT/test_runtime"
echo "core tests passed"
