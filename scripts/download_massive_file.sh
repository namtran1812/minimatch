#!/usr/bin/env bash
set -euo pipefail
if [[ $# -ne 1 ]]; then
  echo "Usage: $0 OBJECT_KEY"
  echo "Example: $0 us_stocks_sip/minute_aggs_v1/2026/07/2026-07-10.csv.gz"
  exit 2
fi
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck disable=SC1091
source "$ROOT/scripts/setup_massive.sh"
KEY="$1"
OUT="$ROOT/data/massive/raw/$(basename "$KEY")"
mkdir -p "$(dirname "$OUT")"
aws s3 cp "s3://${MASSIVE_S3_BUCKET}/${KEY}" "$OUT" --endpoint-url "$MASSIVE_S3_ENDPOINT"
echo "$OUT"
