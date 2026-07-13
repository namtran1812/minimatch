#!/usr/bin/env bash
set -Eeuo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

PORT="${PORT:-10000}"
LIVE_SYMBOL="${LIVE_SYMBOL:-btcusd}"

mkdir -p data/live

PIDS=()

start_adapter() {
  local venue="$1"

  echo "Starting live adapter:"
  echo "  venue=${venue}"
  echo "  symbol=${LIVE_SYMBOL}"

  python3 -u live_feed/live_market.py \
    --venue "$venue" \
    --symbol "$LIVE_SYMBOL" &

  PIDS+=("$!")
}

cleanup() {
  echo "Stopping live adapters..."

  for pid in "${PIDS[@]:-}"; do
    kill "$pid" 2>/dev/null || true
  done

  for pid in "${PIDS[@]:-}"; do
    wait "$pid" 2>/dev/null || true
  done
}

trap cleanup EXIT INT TERM

start_adapter coinbase
start_adapter kraken
start_adapter binance

sleep 5

for pid in "${PIDS[@]}"; do
  if ! kill -0 "$pid" 2>/dev/null; then
    echo "A live adapter exited during startup: pid=$pid" >&2
    exit 1
  fi
done

echo "All public live adapters started."
echo "Starting MiniMatch web server on port ${PORT}"

exec ./build/minimatch_web "$PORT" frontend
