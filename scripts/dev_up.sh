#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

mkdir -p logs
mkdir -p data/fix
mkdir -p data/recordings

LIVE_PORT="${LIVE_PORT:-8090}"
REPLAY_PORT="${REPLAY_PORT:-8091}"
API_PORT="${API_PORT:-8081}"
FIX_PORT="${FIX_PORT:-9876}"

RECORDING="${RECORDING:-data/recordings/btcusd_live_$(date +%Y%m%d_%H%M%S).mmdata}"

LATEST_REPLAY="$(
  ls -t data/recordings/*.mmdata 2>/dev/null \
    | head -1 \
    || true
)"

cleanup() {
  echo
  echo "Stopping MiniMatch services..."

  [[ -n "${API_PID:-}" ]] && kill "$API_PID" 2>/dev/null || true
  [[ -n "${FIX_PID:-}" ]] && kill "$FIX_PID" 2>/dev/null || true
  [[ -n "${LIVE_PID:-}" ]] && kill "$LIVE_PID" 2>/dev/null || true
  [[ -n "${REPLAY_PID:-}" ]] && kill "$REPLAY_PID" 2>/dev/null || true
  [[ -n "${FRONTEND_PID:-}" ]] && kill "$FRONTEND_PID" 2>/dev/null || true

  wait 2>/dev/null || true

  echo "MiniMatch stopped."
}

trap cleanup EXIT INT TERM

echo "Starting MiniMatch..."

FIX_DB_PATH=data/fix/minimatch_fix.sqlite \
FIX_SESSION_ID='MINIMATCH->CLIENT' \
./build/minimatch_api \
  > logs/api.log 2>&1 &

API_PID=$!

./build/minimatch_fix_server \
  "$FIX_PORT" \
  MINIMATCH \
  CLIENT \
  30 \
  data/fix/minimatch_fix.sqlite \
  > logs/fix.log 2>&1 &

FIX_PID=$!

./build/minimatch_live_market_ws \
  "$LIVE_PORT" \
  BTC-USD \
  --record "$RECORDING" \
  > logs/live_market.log 2>&1 &

LIVE_PID=$!

if [[ -n "$LATEST_REPLAY" ]]; then
  ./build/minimatch_market_replay_ws \
    "$REPLAY_PORT" \
    "$LATEST_REPLAY" \
    BTC-USD \
    1.0 \
    > logs/replay.log 2>&1 &

  REPLAY_PID=$!
else
  echo "No replay recording found. Replay server not started."
fi

(
  cd frontend

  VITE_MARKET_WS_URL="ws://127.0.0.1:${LIVE_PORT}" \
  VITE_REPLAY_WS_URL="ws://127.0.0.1:${REPLAY_PORT}" \
  npm run dev
) > logs/frontend.log 2>&1 &

FRONTEND_PID=$!

echo
echo "MiniMatch services"
echo "------------------"
echo "API:        http://127.0.0.1:${API_PORT}"
echo "FIX:        127.0.0.1:${FIX_PORT}"
echo "LIVE WS:    ws://127.0.0.1:${LIVE_PORT}"
echo "REPLAY WS:  ws://127.0.0.1:${REPLAY_PORT}"
echo "Recording:  ${RECORDING}"
echo
echo "Logs:"
echo "  logs/api.log"
echo "  logs/fix.log"
echo "  logs/live_market.log"
echo "  logs/replay.log"
echo "  logs/frontend.log"
echo
echo "Press Ctrl+C to stop everything."

wait
