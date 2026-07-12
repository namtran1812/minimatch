# MiniMatch

MiniMatch is a C++20 electronic exchange and quantitative research platform with a pastel Y2K-inspired frontend. This edition uses Coinbase Advanced Trade as the recommended free public live feed, with Kraken Spot and Binance.US as working alternatives. Alpaca and Massive/Polygon are shown as intentional Coming Soon providers when credentials or data entitlements are unavailable.

MiniMatch is a deterministic C++20 electronic exchange and market-microstructure research platform. It combines an in-memory price-time priority matching engine, advanced order semantics, binary TCP ingress, event sourcing, snapshots, deterministic replay, risk controls, quantitative research tools, live external market-data ingestion, and a browser-based visualization workspace.

## Core capabilities

### Exchange engine

- Multi-symbol price-time priority matching
- Limit, market, immediate-or-cancel, fill-or-kill, and post-only orders
- Cancel and replace with correct priority behavior
- Partial fills and duplicate-order rejection
- Aggregate depth and deterministic state hashing

### Systems engineering

- Boost.Asio TCP gateway and binary protocol
- Append-only event journal
- Snapshot recovery and deterministic replay
- Cache-separated SPSC ingress queue and dedicated matching thread
- Throughput and p50/p95/p99/p99.9 latency instrumentation

### Research and execution

- Inventory-aware market maker
- Momentum and volatility-targeting backtester
- Black–Scholes, Greeks, implied volatility, and Monte Carlo pricing
- Pairs diagnostics, VaR, and Expected Shortfall
- TWAP/VWAP scheduling and implementation-shortfall simulation
- Queue-position and fill modeling

### Live market data

- Coinbase Advanced Trade public Level 2 and trade ingestion
- Kraken Spot public Level 2 and trade ingestion
- Binance.US depth, trade, and book-ticker ingestion
- Provider placeholders for Alpaca and Massive/Polygon
- SQLite and JSONL session recording
- Feed-health, reconnect, and sequence-gap monitoring
- External depth visualization and local paper execution

## Frontend

The frontend uses a clean Y2K-inspired visual system while preserving professional market-data conventions. It has a two-font typography system, consistent type scale, four-metric desktop grid, wide command deck, high-density order-book tables, analytical charts, replay controls, and live-feed monitoring.

See [`docs/DESIGN_SYSTEM.md`](docs/DESIGN_SYSTEM.md).

## Requirements

- CMake 3.20+
- C++20 compiler
- Boost 1.74+
- Python 3.10+ for live-feed adapters

macOS:

```bash
brew install cmake boost
```

## Build

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DMINIMATCH_BUILD_TESTS=ON
cmake --build build -j"$(sysctl -n hw.ncpu)"
```

## Test

```bash
./build/minimatch_tests
ctest --test-dir build --output-on-failure
```

## Launch the dashboard

```bash
./scripts/run_dashboard.sh
```

Open:

```text
http://127.0.0.1:8080
```

## Performance tools

```bash
./build/minimatch_loadgen 1000000 8
./build/minimatch_latency 1000000
./build/minimatch_arena 200000
```

Only report measurements produced on your own hardware. Do not claim a latency improvement without a defined baseline and controlled comparison.

## TCP exchange

Terminal 1:

```bash
rm -f minimatch.events
./build/minimatch_server 9001 minimatch.events
```

Terminal 2:

```bash
./build/minimatch_client 127.0.0.1 9001 new 1 10 1 sell limit 10000 50
./build/minimatch_client 127.0.0.1 9001 new 2 11 1 buy market 0 20
```

Stop the server and replay:

```bash
./build/minimatch_replay minimatch.events
```

## Free live market data

```bash
./scripts/setup_live_feed.sh
./scripts/run_live_coinbase.sh btcusd
# or: ./scripts/run_live_kraken.sh btcusd
# or: ./scripts/run_live_binance.sh btcusd
```

In another terminal:

```bash
./scripts/run_dashboard.sh
```

## Repository structure

```text
apps/          Executable entry points
include/       Public C++ headers
src/           Exchange, risk, research, and pipeline implementation
tests/         GoogleTest suites
frontend/      Browser dashboard
live_feed/     Coinbase, Kraken, Binance.US, and optional Alpaca adapters
scripts/       Build, dashboard, feed, and database utilities
docs/          Architecture and design documentation
```

## Resume-safe description

> Built a deterministic C++20 multi-symbol exchange and market-microstructure research platform with advanced order semantics, binary TCP ingress, event sourcing, snapshot recovery, lock-free command processing, native latency instrumentation, live venue data ingestion, and interactive replay and depth visualization.

## Massive historical data

MiniMatch includes a Massive Flat Files integration for historical U.S. equity research. See [`README-MASSIVE-FLAT-FILES.md`](README-MASSIVE-FLAT-FILES.md).

Quick offline demo:

```bash
python3 scripts/query_historical_db.py --ticker AAPL --limit 10
```
