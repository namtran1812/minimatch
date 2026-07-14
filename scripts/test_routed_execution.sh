#!/usr/bin/env bash
set -Eeuo pipefail

BASE_URL="${BASE_URL:-http://127.0.0.1:8080}"
SYMBOL="${LIVE_SYMBOL:-btcusd}"
QUANTITY="${EXECUTION_QUANTITY:-0.01}"

response="$(
  curl \
    --fail \
    --silent \
    --show-error \
    -X POST \
    -H "Content-Type: application/x-www-form-urlencoded" \
    --data "side=buy&quantity=${QUANTITY}&symbol=${SYMBOL}&fillRatio=1" \
    "${BASE_URL}/api/router/execute"
)"

printf '%s\n' "$response" | python3 -m json.tool

printf '%s\n' "$response" | python3 -c '
import json
import math
import sys

summary = json.load(sys.stdin)

requested = float(summary["requestedQuantity"])
filled = float(summary["filledQuantity"])
remaining = float(summary["remainingQuantity"])

assert summary["side"] == "BUY"
assert requested > 0
assert summary["complete"] is True

assert math.isclose(
    filled,
    requested,
    rel_tol=1e-12,
    abs_tol=1e-9,
)

assert math.isclose(
    remaining,
    0.0,
    rel_tol=1e-12,
    abs_tol=1e-9,
)

assert float(summary["averageFillPrice"]) > 0
assert float(summary["totalNotional"]) > 0
assert float(summary["totalFees"]) >= 0
assert float(summary["totalLatencyMs"]) >= 0
assert summary["children"]

child_total = 0.0

for child in summary["children"]:
    assert child["venue"] in (
        "coinbase",
        "kraken",
        "binance",
    )
    assert child["status"] == "FILLED"
    assert float(child["price"]) > 0
    assert float(child["filledQuantity"]) > 0
    assert float(child["remainingQuantity"]) >= 0
    child_total += float(child["filledQuantity"])

assert math.isclose(
    child_total,
    filled,
    rel_tol=1e-12,
    abs_tol=1e-9,
)

print("PASS: routed execution API")
'
