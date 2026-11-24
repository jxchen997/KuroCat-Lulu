#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
OUT="$ROOT/kurocat"

echo "==> Building KuroCat for Linux (C++17 / GTK3 + Cairo + XInput2 + evdev)..."
g++ -std=c++17 -O2 -Wall -Wextra \
    "$ROOT/src/main.cpp" \
    -o "$OUT" \
    $(pkg-config --cflags --libs gtk+-3.0 cairo x11 xi) \
    -lpthread

strip "$OUT" || true
echo "==> Built successfully: $OUT ($(du -h "$OUT" | cut -f1))"
