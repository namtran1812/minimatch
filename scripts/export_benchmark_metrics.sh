#!/usr/bin/env bash
set -euo pipefail

FILE="${1:-benchmark_results/drop_copy_lookup.txt}"
OUT="${2:-benchmark_results/benchmark_metrics.prom}"

if [[ ! -f "$FILE" ]]; then
  echo "Missing benchmark file: $FILE"
  exit 1
fi

VALUE="$(
  awk '
    /drop-copy single: 100000 events/ {
      active=1
      next
    }

    active && /^avg_lookup_ns=/ {
      sub(/^avg_lookup_ns=/, "")
      print
      exit
    }
  ' "$FILE"
)"

if [[ -z "$VALUE" ]]; then
  echo "Could not find 100K single-match result"
  exit 1
fi

cat > "$OUT" <<METRICS
# HELP minimatch_drop_copy_lookup_100k_single_nanoseconds Latest 100K-row single-match drop-copy lookup benchmark
# TYPE minimatch_drop_copy_lookup_100k_single_nanoseconds gauge
minimatch_drop_copy_lookup_100k_single_nanoseconds $VALUE
METRICS

echo "Wrote $OUT"
