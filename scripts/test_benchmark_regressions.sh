#!/usr/bin/env bash
set -euo pipefail

export BUILD_DIR="${BUILD_DIR:-$(pwd)}"

cd "$(dirname "$0")/.."

OUTPUT_DIR="benchmark_results/ci"

rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

RUNS=1 \
ORDERS=100000 \
SYMBOLS=8 \
OUTPUT_DIR="$OUTPUT_DIR" \
./scripts/run_benchmarks.sh

LATENCY_FILE="$OUTPUT_DIR/latency_1_runs.txt" \
./scripts/check_benchmark_regressions.sh \
  "$OUTPUT_DIR/drop_copy_lookup.txt"

echo "PASS: fresh benchmark regression suite"