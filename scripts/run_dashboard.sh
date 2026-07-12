#!/usr/bin/env bash
set -euo pipefail
PORT="${1:-8080}"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DMINIMATCH_BUILD_TESTS=ON
cmake --build build --target minimatch_web -j"$(sysctl -n hw.ncpu 2>/dev/null || nproc)"
echo "Opening MiniMatch dashboard at http://127.0.0.1:${PORT}"
if command -v open >/dev/null 2>&1; then
  (sleep 1; open "http://127.0.0.1:${PORT}") &
fi
exec ./build/minimatch_web "${PORT}" frontend
