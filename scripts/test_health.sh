#!/usr/bin/env bash
set -Eeuo pipefail

BASE_URL="${BASE_URL:-http://127.0.0.1:8080}"
SYMBOL="${LIVE_SYMBOL:-btcusd}"

response="$(
  curl \
    --fail \
    --silent \
    --show-error \
    "${BASE_URL}/api/health?symbol=${SYMBOL}"
)"

printf '%s\n' "$response" | python3 -m json.tool

printf '%s\n' "$response" | python3 -c '
import json
import sys

health = json.load(sys.stdin)

if health["status"] != "healthy":
    raise SystemExit(
        f"overall status is {health['"'"'status'"'"']}"
    )

for venue in ("coinbase", "kraken", "binance"):
    state = health["venues"][venue]

    if not state["healthy"]:
        raise SystemExit(
            f"{venue} is unhealthy: {state}"
        )

print("PASS: MiniMatch and all live venues are healthy.")
'
