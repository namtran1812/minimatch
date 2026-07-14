#!/usr/bin/env bash
set -Eeuo pipefail

BASE_URL="${BASE_URL:-http://127.0.0.1:8080}"
SYMBOL="${LIVE_SYMBOL:-btcusd}"

response="$(
  curl \
    --fail \
    --silent \
    --show-error \
    "${BASE_URL}/api/market/consolidated?symbol=${SYMBOL}"
)"

printf '%s\n' "$response" | python3 -m json.tool

printf '%s\n' "$response" | python3 -c '
import json
import math
import sys

market = json.load(sys.stdin)

assert market["status"] == "live"
assert market["bestBid"] is not None
assert market["bestAsk"] is not None

bid = float(market["bestBid"]["price"])
ask = float(market["bestAsk"]["price"])
spread = float(market["spread"])
midpoint = float(market["midpoint"])
crossed = bool(market["crossed"])

assert bid > 0
assert ask > 0
assert math.isfinite(bid)
assert math.isfinite(ask)
assert math.isfinite(spread)
assert math.isfinite(midpoint)

expected_spread = ask - bid
expected_midpoint = (bid + ask) / 2.0
expected_crossed = expected_spread < 0

assert math.isclose(
    spread,
    expected_spread,
    rel_tol=1e-12,
    abs_tol=1e-6,
), (
    f"spread mismatch: reported={spread}, expected={expected_spread}"
)

assert math.isclose(
    midpoint,
    expected_midpoint,
    rel_tol=1e-12,
    abs_tol=1e-6,
), (
    f"midpoint mismatch: reported={midpoint}, expected={expected_midpoint}"
)

assert crossed == expected_crossed, (
    f"crossed mismatch: reported={crossed}, expected={expected_crossed}"
)

venues = market["venues"]

for venue in ("coinbase", "kraken", "binance"):
    assert venue in venues
    quote = venues[venue]

    if quote["healthy"]:
        venue_bid = float(quote["bid"])
        venue_ask = float(quote["ask"])

        assert venue_bid > 0
        assert venue_ask > 0
        assert venue_ask > venue_bid, (
            f"{venue} local book is crossed: "
            f"bid={venue_bid}, ask={venue_ask}"
        )

print(
    "PASS:",
    f"best bid {bid} on {market['"'"'bestBid'"'"']['"'"'venue'"'"']},",
    f"best ask {ask} on {market['"'"'bestAsk'"'"']['"'"'venue'"'"']},",
    f"spread {spread},",
    f"crossed={crossed}"
)
'
