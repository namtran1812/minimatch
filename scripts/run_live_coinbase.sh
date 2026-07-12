#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
SYMBOL="${1:-btcusd}"
[[ -d .venv-live ]] || ./scripts/setup_live_feed.sh
source .venv-live/bin/activate
exec python live_feed/live_market.py --venue coinbase --symbol "$SYMBOL"
