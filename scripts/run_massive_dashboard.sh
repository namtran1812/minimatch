#!/usr/bin/env bash
set -Eeuo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

TICKER="${1:-AAPL}"
YEAR="${2:-$(date +%Y)}"
PORT="${3:-8080}"

./scripts/auto_massive.sh "$TICKER" "$YEAR"
exec ./scripts/run_dashboard.sh "$PORT"
