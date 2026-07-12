#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ENV_FILE="$ROOT/.env"
if [[ ! -f "$ENV_FILE" ]]; then
  echo "Missing $ENV_FILE"
  echo "Copy .env.example to .env and add newly rotated Massive credentials."
  exit 1
fi
set -a
# shellcheck disable=SC1090
source "$ENV_FILE"
set +a
: "${MASSIVE_ACCESS_KEY_ID:?Missing MASSIVE_ACCESS_KEY_ID}"
: "${MASSIVE_SECRET_ACCESS_KEY:?Missing MASSIVE_SECRET_ACCESS_KEY}"
export MASSIVE_S3_ENDPOINT="${MASSIVE_S3_ENDPOINT:-https://files.massive.com}"
export MASSIVE_S3_BUCKET="${MASSIVE_S3_BUCKET:-flatfiles}"
export AWS_DEFAULT_REGION="${AWS_DEFAULT_REGION:-us-east-1}"
export AWS_ACCESS_KEY_ID="$MASSIVE_ACCESS_KEY_ID"
export AWS_SECRET_ACCESS_KEY="$MASSIVE_SECRET_ACCESS_KEY"
echo "Massive Flat Files credentials loaded for this shell."
echo "Endpoint: $MASSIVE_S3_ENDPOINT"
echo "Bucket:   $MASSIVE_S3_BUCKET"
