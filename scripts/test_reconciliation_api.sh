#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

API="./build/minimatch_api"
PORT="${MINIMATCH_TEST_API_PORT:-18081}"
URL="http://127.0.0.1:${PORT}"

API_PID=""

cleanup() {
  if [[ -n "${API_PID}" ]]; then
    kill "${API_PID}" 2>/dev/null || true
    wait "${API_PID}" 2>/dev/null || true
  fi
}

trap cleanup EXIT

if lsof -nP \
  -iTCP:"${PORT}" \
  -sTCP:LISTEN \
  >/dev/null 2>&1
then
  echo "Test port ${PORT} is already in use"
  exit 1
fi

MINIMATCH_API_PORT="${PORT}" \
  "${API}" \
  >/tmp/minimatch_reconciliation_test.log \
  2>&1 &

API_PID=$!

for _ in {1..50}; do
  if curl -fsS \
    "${URL}/api/reconciliation" \
    >/tmp/minimatch_reconciliation.json \
    2>/dev/null
  then
    break
  fi

  if ! kill -0 "${API_PID}" 2>/dev/null; then
    echo "Test API exited unexpectedly"
    cat /tmp/minimatch_reconciliation_test.log
    exit 1
  fi

  sleep 0.1
done

python3 - <<'PY'
import json
from pathlib import Path

p = Path("/tmp/minimatch_reconciliation.json")

if not p.exists():
    raise SystemExit(
        "Reconciliation endpoint never became available"
    )

data = json.loads(
    p.read_text()
)

required = {
    "parentCount",
    "consistentParents",
    "legacyUnverifiableParents",
    "inconsistentParents",
    "allVerifiableParentsConsistent",
    "positionsConsistent",
    "overallConsistent",
}

missing = required - data.keys()

if missing:
    raise SystemExit(
        f"Missing fields: {sorted(missing)}"
    )

if not data["allVerifiableParentsConsistent"]:
    raise SystemExit(
        "Verifiable parent reconciliation failed"
    )

if not data["positionsConsistent"]:
    raise SystemExit(
        "Position reconciliation failed"
    )

if not data["overallConsistent"]:
    raise SystemExit(
        "Overall reconciliation failed"
    )

print(
    "PASS: global reconciliation API is healthy"
)

print(
    f"parents={data['parentCount']} "
    f"consistent={data['consistentParents']} "
    f"legacy={data['legacyUnverifiableParents']} "
    f"inconsistent={data['inconsistentParents']}"
)
PY
