#!/usr/bin/env bash
set -Eeuo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

PORT="${PORT:-10000}"
LIVE_PROVIDER="${LIVE_PROVIDER:-coinbase}"
LIVE_SYMBOL="${LIVE_SYMBOL:-btcusd}"

ADAPTER_PID=""

cleanup() {
  if [[ -n "${ADAPTER_PID}" ]]; then
    kill "${ADAPTER_PID}" 2>/dev/null || true
    wait "${ADAPTER_PID}" 2>/dev/null || true
  fi
}

trap cleanup EXIT INT TERM

case "$LIVE_PROVIDER" in
  coinbase|kraken|binance)
    echo "Starting live market adapter:"
    echo "  provider=${LIVE_PROVIDER}"
    echo "  symbol=${LIVE_SYMBOL}"

    python3 -u live_feed/live_market.py \
      --venue "$LIVE_PROVIDER" \
      --symbol "$LIVE_SYMBOL" &

    ADAPTER_PID=$!
    ;;

  none)
    echo "Live market adapter disabled."
    ;;

  *)
    echo "Unsupported LIVE_PROVIDER=${LIVE_PROVIDER}" >&2
    exit 1
    ;;
esac

sleep 2

if [[ -n "${ADAPTER_PID}" ]] && ! kill -0 "${ADAPTER_PID}" 2>/dev/null; then
  echo "Live adapter exited during startup." >&2
  exit 1
fi

echo "Starting MiniMatch web server on port ${PORT}"

exec ./build/minimatch_web \
  "$PORT" \
  frontend
