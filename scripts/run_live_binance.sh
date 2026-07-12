#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
SYMBOL="${1:-btcusd}"
export BINANCE_REST_BASE="${BINANCE_REST_BASE:-https://api.binance.us}"
export BINANCE_WS_BASE="${BINANCE_WS_BASE:-wss://stream.binance.us:9443}"
[[ -d .venv-live ]] || ./scripts/setup_live_feed.sh
source .venv-live/bin/activate
exec python live_feed/live_market.py --venue binance --symbol "$SYMBOL"
