#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck disable=SC1091
source "$ROOT/scripts/setup_massive.sh"
command -v aws >/dev/null || { echo "AWS CLI missing. Install with: brew install awscli"; exit 1; }
aws s3 ls "s3://${MASSIVE_S3_BUCKET}/" --endpoint-url "$MASSIVE_S3_ENDPOINT" | head -30
echo "PASS: Massive Flat Files connection is working."
