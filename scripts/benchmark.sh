#!/usr/bin/env bash
set -euo pipefail
COUNT="${1:-1000000}"
mkdir -p results
./build/minimatch_latency "$COUNT" | tee "results/latency_${COUNT}.txt"
./build/minimatch_loadgen "$COUNT" 8 | tee "results/loadgen_${COUNT}.txt"
