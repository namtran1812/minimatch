#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 -m venv .venv-live
source .venv-live/bin/activate
python -m pip install --upgrade pip
python -m pip install -r live_feed/requirements.txt
echo "Live-feed environment ready."
