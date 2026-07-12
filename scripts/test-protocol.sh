#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/build"
mkdir -p "$OUT"

cc -std=c11 -Wall -Wextra -I"$ROOT/firmware/shared" \
  "$ROOT/firmware/shared/cai_protocol.c" \
  "$ROOT/firmware/tests/test_protocol.c" \
  -o "$OUT/test_protocol"

"$OUT/test_protocol"
echo "protocol tests passed"
