#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

echo "Configuring..."
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release

echo "Building..."
cmake --build build \
  -j"$(sysctl -n hw.ncpu)"

echo "Building frontend..."
(
  cd frontend
  npm run build
)

echo "Running tests..."
ctest \
  --test-dir build \
  --output-on-failure

echo "Starting services..."
exec ./scripts/dev_up.sh
