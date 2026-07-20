#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

API="http://127.0.0.1:8081"

assert_json() {
  local json="$1"
  local expression="$2"
  local description="$3"

  JSON_PAYLOAD="$json" \
  python3 - "$expression" "$description" <<'PY_ASSERT'
import json
import os
import sys

expression = sys.argv[1]
description = sys.argv[2]

try:
    data = json.loads(
        os.environ["JSON_PAYLOAD"]
    )
except Exception as exc:
    print(
        f"ASSERTION ERROR: invalid JSON: {exc}"
    )
    sys.exit(1)

try:
    ok = bool(
        eval(
            expression,
            {},
            {"data": data},
        )
    )
except Exception as exc:
    print(
        f"ASSERTION ERROR: "
        f"{description}: {exc}"
    )
    sys.exit(1)

if not ok:
    print(
        f"ASSERTION FAILED: "
        f"{description}"
    )
    print(
        json.dumps(
            data,
            indent=2,
        )
    )
    sys.exit(1)

print(
    f"PASS: {description}"
)
PY_ASSERT
}


echo "========================================"
echo " MiniMatch End-to-End Verification"
echo "========================================"

echo
echo "[1] API health"

if curl -fsS "$API/health" >/dev/null 2>&1; then
  curl -fsS "$API/health"
elif curl -fsS "$API/api/health" >/dev/null 2>&1; then
  curl -fsS "$API/api/health"
elif curl -fsS "$API/api/system" >/dev/null 2>&1; then
  curl -fsS "$API/api/system"
else
  echo "ERROR: MiniMatch API is not reachable on $API"
  echo
  echo "Check:"
  echo "  lsof -nP -iTCP:8081 -sTCP:LISTEN"
  echo "  tail -100 logs/api.log"
  exit 1
fi

echo

echo
echo "[2] Matching engine order lifecycle"

ORDER_ID="$(date +%s%N | cut -c1-15)"

curl -fsS \
  -X POST \
  "$API/api/orders" \
  -H 'Content-Type: application/json' \
  -d "{
    \"orderId\": $ORDER_ID,
    \"clientId\": 99,
    \"side\": \"buy\",
    \"type\": \"limit\",
    \"price\": 10000,
    \"quantity\": 10,
    \"symbol\": 1
  }"

echo

curl -fsS \
  "$API/api/market/book/1"

echo

curl -fsS \
  -X POST \
  "$API/api/cancel" \
  -H 'Content-Type: application/json' \
  -d "{
    \"orderId\": $ORDER_ID,
    \"clientId\": 99,
    \"symbol\": 1
  }"

echo
echo "Matching lifecycle OK"

echo
echo "[3] Historical backtest + OMS"

BACKTEST_RESULT="$(
  curl -fsS \
    -X POST \
    "$API/api/backtest" \
    -H 'Content-Type: application/json' \
    -d '{
      "symbol": "btcusd",
      "side": "buy",
      "quantity": 2,
      "algorithm": "TWAP",
      "slices": 2,
      "durationSeconds": 2,
      "takerFeeBps": 5
    }'
)"

echo "$BACKTEST_RESULT"

assert_json   "$BACKTEST_RESULT"   '"parentOrderId" in data and data["requestedQuantity"] > 0'   "backtest created OMS parent"

PARENT_ID="$(
  printf '%s' "$BACKTEST_RESULT" \
  | python3 -c '
import json
import sys
print(json.load(sys.stdin)["parentOrderId"])
'
)"

echo "Parent ID: $PARENT_ID"

curl -fsS \
  "$API/api/oms/parents/$PARENT_ID/children"

echo

curl -fsS \
  "$API/api/oms/parents/$PARENT_ID/fills"

echo
echo "Backtest + OMS OK"

echo
echo "[4] Smart order router preview"

curl -fsS \
  -X POST \
  "$API/api/router/preview" \
  -H 'Content-Type: application/json' \
  -d '{
    "symbol": "btcusd",
    "side": "buy",
    "quantity": 0.1,
    "maxSlippageBps": 100,
    "maxVenueCount": 3,
    "allOrNone": false
  }'

echo
echo "Router preview OK"

echo
echo "[5] Routed execution + SQLite persistence"

EXECUTION_RESULT="$(
  curl -fsS \
    -X POST \
    "$API/api/executions" \
    -H 'Content-Type: application/json' \
    -d '{
      "symbol": "btcusd",
      "side": "buy",
      "quantity": 0.1,
      "maxSlippageBps": 100,
      "maxVenueCount": 3,
      "fillRatio": 0.8,
      "rejectionProbability": 0,
      "baseLatencyMs": 1,
      "latencyJitterMs": 2,
      "seed": 42
    }'
)"

echo "$EXECUTION_RESULT"

assert_json   "$EXECUTION_RESULT"   '"executionId" in data and data["requestedQuantity"] > 0'   "routed execution persisted"

EXECUTION_ID="$(
  printf '%s' "$EXECUTION_RESULT" \
  | python3 -c '
import json
import sys
print(json.load(sys.stdin)["executionId"])
'
)"

echo "Execution ID: $EXECUTION_ID"

curl -fsS \
  "$API/api/executions/$EXECUTION_ID"

echo
echo "Execution persistence OK"

echo
echo "[6] FIX persistence"

curl -fsS \
  "$API/api/fix/session"

echo

curl -fsS \
  "$API/api/fix/messages"

echo
echo "FIX observability OK"

echo
echo "[7] System metrics"

curl -fsS \
  "$API/api/system"

echo

echo
echo "[8] Binary replay determinism"

if [[ -f minimatch.events ]]; then
  ./build/minimatch_replay \
    minimatch.events \
    > /tmp/minimatch_replay_1.txt

  ./build/minimatch_replay \
    minimatch.events \
    > /tmp/minimatch_replay_2.txt

  diff -u \
    /tmp/minimatch_replay_1.txt \
    /tmp/minimatch_replay_2.txt

  cat /tmp/minimatch_replay_1.txt

  echo "Deterministic replay OK"
else
  echo "minimatch.events not found; skipping exchange replay test"
fi

echo
echo "[9] Core tests"

ctest \
  --test-dir build \
  --output-on-failure

echo
echo "========================================"
echo " MiniMatch verification PASSED"
echo "========================================"
