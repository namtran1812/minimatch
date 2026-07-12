#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck disable=SC1091
source "$ROOT/scripts/setup_massive.sh"
DATASET="${1:-us_stocks_sip/minute_aggs_v1}"
YEAR="${2:-$(date +%Y)}"
aws s3 ls "s3://${MASSIVE_S3_BUCKET}/${DATASET}/${YEAR}/" \
  --recursive --endpoint-url "$MASSIVE_S3_ENDPOINT" | tail -40
