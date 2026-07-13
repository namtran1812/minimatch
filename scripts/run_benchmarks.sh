#!/usr/bin/env bash
set -Eeuo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

RUNS="${RUNS:-5}"
ORDERS="${ORDERS:-1000000}"
SYMBOLS="${SYMBOLS:-8}"
OUTPUT_DIR="${OUTPUT_DIR:-benchmark_results}"

mkdir -p "$OUTPUT_DIR"

if [[ ! -x build/minimatch_latency ]] ||
   [[ ! -x build/minimatch_loadgen ]]; then
  echo "Benchmark binaries are missing. Building them..."
  cmake --build build \
    --target minimatch_latency minimatch_loadgen \
    --parallel 2
fi

{
  for run in $(seq 1 "$RUNS"); do
    echo "===== latency run $run ====="
    ./build/minimatch_latency "$ORDERS"
  done
} | tee "$OUTPUT_DIR/latency_${RUNS}_runs.txt"

{
  for run in $(seq 1 "$RUNS"); do
    echo "===== loadgen run $run ====="
    ./build/minimatch_loadgen "$ORDERS" "$SYMBOLS"
  done
} | tee "$OUTPUT_DIR/loadgen_${RUNS}_runs.txt"

echo
echo "Benchmark results written to $OUTPUT_DIR"
