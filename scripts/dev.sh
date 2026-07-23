#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

LOG_DIR="$ROOT/.dev-logs"
mkdir -p "$LOG_DIR"

PIDS=()

cleanup() {
  echo
  echo "[MiniMatch] stopping services..."

  for pid in "${PIDS[@]:-}"; do
    kill "$pid" 2>/dev/null || true
  done

  wait 2>/dev/null || true

  echo "[MiniMatch] stopped."
}

trap cleanup EXIT INT TERM

start_service() {
  local name="$1"
  shift

  echo "[MiniMatch] starting $name"

  "$@" \
    >"$LOG_DIR/$name.log" \
    2>&1 &

  PIDS+=("$!")
}

echo "[MiniMatch] root: $ROOT"

start_service \
  api \
  "$ROOT/build/minimatch_api" \
  8081

start_service \
  live-market \
  "$ROOT/build/minimatch_live_market_ws" \
  8089 \
  BTC-USD

if [[ -f "$ROOT/data/market/btcusd.market.bin" ]]; then
  start_service \
    replay \
    "$ROOT/build/minimatch_market_replay_ws" \
    8091 \
    "$ROOT/data/market/btcusd.market.bin" \
    BTC-USD \
    1
else
  echo "[MiniMatch] replay recording missing; skipping 8091"
fi

start_service \
  drop-copy \
  "$ROOT/build/minimatch_drop_copy_ws" \
  8092

echo "[MiniMatch] starting frontend"

cd "$ROOT/frontend"

npm run dev -- --host 127.0.0.1 &
PIDS+=("$!")

sleep 2

echo
echo "=========================================="
echo " MiniMatch development stack"
echo "=========================================="
echo " Frontend     http://127.0.0.1:5173"
echo " API          http://127.0.0.1:8081"
echo " Live market  ws://127.0.0.1:8089"
echo " Replay       ws://127.0.0.1:8091"
echo " Drop copy    ws://127.0.0.1:8092"
echo
echo " Logs: $LOG_DIR"
echo " Ctrl+C to stop everything"
echo "=========================================="
echo

wait
