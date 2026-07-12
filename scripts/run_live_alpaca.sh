#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
SYMBOL="${1:-AAPL}"
[[ -d .venv-live ]] || ./scripts/setup_live_feed.sh
source .venv-live/bin/activate
: "${APCA_API_KEY_ID:?Set APCA_API_KEY_ID}"
: "${APCA_API_SECRET_KEY:?Set APCA_API_SECRET_KEY}"
exec python live_feed/live_market.py --venue alpaca --symbol "$SYMBOL"
