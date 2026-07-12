#!/usr/bin/env bash
set -Eeuo pipefail

# MiniMatch Massive Flat Files automation
#
# Usage:
#   ./scripts/auto_massive.sh AAPL
#   ./scripts/auto_massive.sh NVDA 2026
#
# Requirements:
#   - .env exists with MASSIVE_ACCESS_KEY_ID and MASSIVE_SECRET_ACCESS_KEY
#   - aws CLI installed
#   - python3 available

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

TICKER="${1:-AAPL}"
YEAR="${2:-$(date +%Y)}"

ENV_FILE="${MASSIVE_ENV_FILE:-$PROJECT_ROOT/.env}"
ENDPOINT_DEFAULT="https://files.massive.com"
BUCKET_DEFAULT="flatfiles"
PREFIX="us_stocks_sip/minute_aggs_v1/${YEAR}"

RAW_DIR="$PROJECT_ROOT/data/massive/raw"
HIST_DIR="$PROJECT_ROOT/data/historical"
DB_PATH="$HIST_DIR/market_history.db"
JSON_PATH="$HIST_DIR/massive_${TICKER,,}.json"
LOG_DIR="$PROJECT_ROOT/results"
LOG_FILE="$LOG_DIR/massive_${TICKER,,}_$(date +%Y%m%d_%H%M%S).log"

mkdir -p "$RAW_DIR" "$HIST_DIR" "$LOG_DIR"

log() {
  printf '[%s] %s\n' "$(date '+%Y-%m-%d %H:%M:%S')" "$*" | tee -a "$LOG_FILE"
}

fail() {
  log "ERROR: $*"
  exit 1
}

if [[ ! -f "$ENV_FILE" ]]; then
  cat >&2 <<EOF
Missing $ENV_FILE.

Create it with:
MASSIVE_ACCESS_KEY_ID=...
MASSIVE_SECRET_ACCESS_KEY=...
MASSIVE_S3_ENDPOINT=https://files.massive.com
MASSIVE_S3_BUCKET=flatfiles
AWS_DEFAULT_REGION=us-east-1
EOF
  exit 1
fi

set -a
# shellcheck disable=SC1090
source "$ENV_FILE"
set +a

: "${MASSIVE_ACCESS_KEY_ID:?Missing MASSIVE_ACCESS_KEY_ID in $ENV_FILE}"
: "${MASSIVE_SECRET_ACCESS_KEY:?Missing MASSIVE_SECRET_ACCESS_KEY in $ENV_FILE}"

export AWS_ACCESS_KEY_ID="$MASSIVE_ACCESS_KEY_ID"
export AWS_SECRET_ACCESS_KEY="$MASSIVE_SECRET_ACCESS_KEY"
export AWS_DEFAULT_REGION="${AWS_DEFAULT_REGION:-us-east-1}"

MASSIVE_S3_ENDPOINT="${MASSIVE_S3_ENDPOINT:-$ENDPOINT_DEFAULT}"
MASSIVE_S3_BUCKET="${MASSIVE_S3_BUCKET:-$BUCKET_DEFAULT}"

command -v aws >/dev/null 2>&1 || fail "aws CLI is not installed. Run: brew install awscli"
command -v python3 >/dev/null 2>&1 || fail "python3 is not installed"
[[ -f scripts/import_massive_minute_aggs.py ]] || \
  fail "scripts/import_massive_minute_aggs.py was not found"

log "Checking Massive Flat Files access..."
aws s3 ls "s3://${MASSIVE_S3_BUCKET}/" \
  --endpoint-url "$MASSIVE_S3_ENDPOINT" >/dev/null \
  || fail "Could not access Massive Flat Files. Check the rotated credentials."

log "Finding the latest minute-aggregate file under ${PREFIX}/..."
LATEST_KEY="$(
  aws s3 ls "s3://${MASSIVE_S3_BUCKET}/${PREFIX}/" \
    --recursive \
    --endpoint-url "$MASSIVE_S3_ENDPOINT" |
  awk '$4 ~ /\.csv\.gz$/ {print $4}' |
  sort |
  tail -n 1
)"

[[ -n "$LATEST_KEY" ]] || \
  fail "No .csv.gz files found under s3://${MASSIVE_S3_BUCKET}/${PREFIX}/"

FILENAME="$(basename "$LATEST_KEY")"
LOCAL_FILE="$RAW_DIR/$FILENAME"

if [[ -f "$LOCAL_FILE" ]]; then
  log "Using existing download: $LOCAL_FILE"
else
  log "Downloading s3://${MASSIVE_S3_BUCKET}/${LATEST_KEY}"
  aws s3 cp \
    "s3://${MASSIVE_S3_BUCKET}/${LATEST_KEY}" \
    "$LOCAL_FILE" \
    --endpoint-url "$MASSIVE_S3_ENDPOINT"
fi

log "Importing ${TICKER} into ${DB_PATH}..."
python3 scripts/import_massive_minute_aggs.py \
  "$LOCAL_FILE" \
  --ticker "$TICKER" \
  --json "$JSON_PATH" |
  tee -a "$LOG_FILE"

if [[ -f scripts/query_historical_db.py ]]; then
  log "Validating imported rows..."
  python3 scripts/query_historical_db.py \
    --ticker "$TICKER" \
    --limit 5 |
    tee -a "$LOG_FILE"
fi

log "Completed successfully."
log "Raw file: $LOCAL_FILE"
log "SQLite database: $DB_PATH"
log "Frontend JSON: $JSON_PATH"
log "Log: $LOG_FILE"

cat <<EOF

Next:
  ./scripts/run_dashboard.sh

Then open:
  http://127.0.0.1:8080

Select:
  Live Market -> Massive Historical Flat Files -> ${TICKER}
EOF
