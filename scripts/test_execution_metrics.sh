#!/usr/bin/env bash
set -Eeuo pipefail

BASE_URL="${BASE_URL:-http://127.0.0.1:8080}"
SYMBOL="${LIVE_SYMBOL:-btcusd}"

before="$(
  curl \
    --fail \
    --silent \
    --show-error \
    "${BASE_URL}/metrics?symbol=${SYMBOL}"
)"

before_requests="$(
  printf '%s\n' "$before" |
  awk '$1 == "minimatch_execution_requests_total" {print $2}'
)"

before_requests="${before_requests:-0}"

curl \
  --fail \
  --silent \
  --show-error \
  -X POST \
  -H "Content-Type: application/x-www-form-urlencoded" \
  --data "side=buy&quantity=0.01&symbol=${SYMBOL}&fillRatio=1&rejectionProbability=0&baseLatencyMs=1&latencyJitterMs=0&seed=7" \
  "${BASE_URL}/api/router/execute" \
  >/dev/null

after="$(
  curl \
    --fail \
    --silent \
    --show-error \
    "${BASE_URL}/metrics?symbol=${SYMBOL}"
)"

printf '%s\n' "$after" |
python3 -c '
import sys

required = {
    "minimatch_execution_requests_total",
    "minimatch_execution_completed_total",
    "minimatch_execution_partial_total",
    "minimatch_execution_rejected_children_total",
    "minimatch_execution_requested_quantity_total",
    "minimatch_execution_filled_quantity_total",
    "minimatch_execution_fees_total",
    "minimatch_execution_latency_milliseconds_total",
    "minimatch_execution_fill_ratio",
    "minimatch_execution_average_latency_milliseconds",
}

found = set()

for line in sys.stdin:
    if line.startswith("#") or not line.strip():
        continue

    name = line.split()[0]

    if name in required:
        found.add(name)

missing = required - found

assert not missing, f"missing execution metrics: {sorted(missing)}"

print("PASS: all execution metrics are exposed")
'

after_requests="$(
  printf '%s\n' "$after" |
  awk '$1 == "minimatch_execution_requests_total" {print $2}'
)"

python3 - "$before_requests" "$after_requests" <<'PY'
import sys

before = float(sys.argv[1])
after = float(sys.argv[2])

assert after >= before + 1, (
    f"request counter did not increase: before={before}, after={after}"
)

print(
    "PASS:",
    f"execution request counter increased from {before} to {after}",
)
PY
