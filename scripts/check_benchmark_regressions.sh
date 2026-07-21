#!/usr/bin/env bash
set -euo pipefail

FILE="${1:-benchmark_results/drop_copy_lookup.txt}"

if [[ ! -f "$FILE" ]]; then
  echo "Benchmark file not found: $FILE"
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
  echo "Could not find 100K single-match benchmark"
  exit 1
fi

MAX_NS="${MAX_DROP_COPY_SINGLE_100K_NS:-50000}"

python3 - "$VALUE" "$MAX_NS" <<'PY'
import sys

value = float(sys.argv[1])
limit = float(sys.argv[2])

print(
    f"drop_copy_single_100k_ns={value:.2f}"
)
print(
    f"threshold_ns={limit:.2f}"
)

if value > limit:
    raise SystemExit(
        "FAIL: drop-copy lookup regression"
    )

print(
    "PASS: drop-copy lookup within threshold"
)
PY


LATENCY_FILE="${LATENCY_FILE:-benchmark_results/latency_1_runs.txt}"

if [[ ! -f "$LATENCY_FILE" ]]; then
  echo "Latency benchmark file not found: $LATENCY_FILE"
  exit 1
fi

LATENCY_LINE="$(
  grep '^orders=' "$LATENCY_FILE" |
  tail -1
)"

if [[ -z "$LATENCY_LINE" ]]; then
  echo "Could not find latency benchmark result"
  exit 1
fi

THROUGHPUT="$(
  printf '%s\n' "$LATENCY_LINE" |
  awk '{
    for (i = 1; i <= NF; ++i) {
      if ($i ~ /^throughput=/) {
        sub(/^throughput=/, "", $i)
        print $i
        exit
      }
    }
  }'
)"

P99_NS="$(
  printf '%s\n' "$LATENCY_LINE" |
  awk '{
    for (i = 1; i <= NF; ++i) {
      if ($i ~ /^p99_ns=/) {
        sub(/^p99_ns=/, "", $i)
        print $i
        exit
      }
    }
  }'
)"

MIN_THROUGHPUT="${MIN_MATCHING_THROUGHPUT:-4000000}"
MAX_P99_NS="${MAX_MATCHING_P99_NS:-1000}"

python3 \
  - "$THROUGHPUT" \
  "$P99_NS" \
  "$MIN_THROUGHPUT" \
  "$MAX_P99_NS" <<'PY2'
import sys

throughput = float(sys.argv[1])
p99_ns = float(sys.argv[2])
min_throughput = float(sys.argv[3])
max_p99_ns = float(sys.argv[4])

print(
    f"matching_throughput={throughput:.2f}"
)
print(
    f"minimum_throughput={min_throughput:.2f}"
)
print(
    f"matching_p99_ns={p99_ns:.2f}"
)
print(
    f"maximum_p99_ns={max_p99_ns:.2f}"
)

if throughput < min_throughput:
    raise SystemExit(
        "FAIL: matching throughput regression"
    )

if p99_ns > max_p99_ns:
    raise SystemExit(
        "FAIL: matching p99 latency regression"
    )

print(
    "PASS: matching engine within thresholds"
)
PY2
