#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
[[ -d .venv-live ]] || ./scripts/setup_live_feed.sh
source .venv-live/bin/activate
python - <<'PY'
import asyncio, json
import websockets

async def test(name, url, subscribe):
    print(f"Testing {name}: {url}")
    try:
        async with websockets.connect(url, open_timeout=15, ping_interval=20, ping_timeout=20) as ws:
            for message in subscribe:
                await ws.send(json.dumps(message))
            raw = await asyncio.wait_for(ws.recv(), timeout=20)
            print(f"PASS {name}: {raw[:180]}")
            return True
    except Exception as exc:
        print(f"FAIL {name}: {exc}")
        return False

async def main():
    results = []
    results.append(await test(
        "Coinbase",
        "wss://advanced-trade-ws.coinbase.com",
        [{"type":"subscribe","product_ids":["BTC-USD"],"channel":"level2"}],
    ))
    results.append(await test(
        "Kraken",
        "wss://ws.kraken.com/v2",
        [{"method":"subscribe","params":{"channel":"ticker","symbol":["BTC/USD"],"snapshot":True}}],
    ))
    if not any(results):
        raise SystemExit(1)

asyncio.run(main())
PY
