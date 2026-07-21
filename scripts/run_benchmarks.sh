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
   [[ ! -x build/minimatch_loadgen ]] ||
   [[ ! -x build/minimatch_drop_copy_benchmark ]]; then
  echo "Benchmark binaries are missing. Building them..."
  cmake --build build \
    --target \
      minimatch_latency \
      minimatch_loadgen \
      minimatch_drop_copy_benchmark \
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

{
  echo "===== DROP-COPY MULTI-MATCH ====="

  for events in 1000 10000 100000; do
    echo "===== drop-copy multi: $events events ====="
    ./build/minimatch_drop_copy_benchmark "$events"
  done

  echo
  echo "===== DROP-COPY SINGLE-MATCH ====="

  for events in 1000 10000 100000; do
    echo "===== drop-copy single: $events events ====="
    ./build/minimatch_drop_copy_benchmark "$events" single
  done
} | tee "$OUTPUT_DIR/drop_copy_lookup.txt"

echo
echo "Benchmark results written to $OUTPUT_DIR"
