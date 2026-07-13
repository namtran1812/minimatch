#!/usr/bin/env bash
set -Eeuo pipefail

cd "$(dirname "$0")/.."

PORT="${PORT:-8080}"

echo "Starting MiniMatch on 0.0.0.0:${PORT}"

exec ./build/minimatch_web \
  "$PORT" \
  frontend
