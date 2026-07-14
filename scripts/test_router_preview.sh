#!/usr/bin/env bash
set -Eeuo pipefail

BASE_URL="${BASE_URL:-http://127.0.0.1:8080}"
SYMBOL="${LIVE_SYMBOL:-btcusd}"
QUANTITY="${ROUTE_QUANTITY:-0.01}"

for side in buy sell; do
  echo "===== $side ====="

  response="$(
    curl \
      --fail \
      --silent \
      --show-error \
      -X POST \
      -H "Content-Type: application/x-www-form-urlencoded" \
      --data "side=${side}&quantity=${QUANTITY}&symbol=${SYMBOL}" \
      "${BASE_URL}/api/router/preview"
  )"

  printf '%s\n' "$response" | python3 -m json.tool

  printf '%s\n' "$response" | python3 -c '
import json
import math
import sys

plan = json.load(sys.stdin)

assert plan["side"] in ("BUY", "SELL")
assert float(plan["requestedQuantity"]) > 0
assert float(plan["routedQuantity"]) >= 0
assert isinstance(plan["complete"], bool)
assert isinstance(plan["legs"], list)

total_quantity = sum(
    float(leg["quantity"])
    for leg in plan["legs"]
)

assert math.isclose(
    total_quantity,
    float(plan["routedQuantity"]),
    rel_tol=1e-12,
    abs_tol=1e-9,
)

for leg in plan["legs"]:
    assert leg["venue"] in (
        "coinbase",
        "kraken",
        "binance",
    )
    assert float(leg["price"]) > 0
    assert float(leg["quantity"]) > 0
    assert float(leg["estimatedFee"]) >= 0
    assert float(leg["effectivePrice"]) > 0
    assert float(leg["latencyMs"]) >= 0
    assert float(leg["takerFeeBps"]) >= 0
    assert float(leg["latencyCostBpsPerMs"]) >= 0

print("PASS:", plan["side"], len(plan["legs"]), "legs")
'
done
