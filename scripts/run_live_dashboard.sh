#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
VENUE="${1:-coinbase}"
SYMBOL="${2:-btcusd}"
PORT="${3:-8080}"
[[ -d build ]] || cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DMINIMATCH_BUILD_TESTS=ON
cmake --build build --target minimatch_web -j"$(sysctl -n hw.ncpu 2>/dev/null || nproc)"
[[ -d .venv-live ]] || ./scripts/setup_live_feed.sh
source .venv-live/bin/activate
python live_feed/live_market.py --venue "$VENUE" --symbol "$SYMBOL" &
FEED_PID=$!
cleanup(){ kill "$FEED_PID" 2>/dev/null || true; }
trap cleanup EXIT INT TERM
( sleep 1; command -v open >/dev/null && open "http://127.0.0.1:${PORT}" || true ) &
./build/minimatch_web "$PORT" frontend
