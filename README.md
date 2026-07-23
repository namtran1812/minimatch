# MiniMatch

MiniMatch is a production-style electronic trading systems project built in modern C++. It combines a low-latency matching engine, order management, risk controls, execution algorithms, smart order routing, FIX 4.4 connectivity, live multi-venue crypto market data, persistent execution history, deterministic market replay, observability, and a React trading terminal.

The project is designed to model the architecture of a modern exchange and execution platform from market ingestion through routing and post-trade analysis.

---

## Highlights

- Price-time priority matching engine
- Limit, market, IOC, FOK, and post-only orders
- Cancel and replace/amend workflows
- Multi-symbol order books
- Deterministic event logging and replay
- Risk checks and kill-switch logic
- Parent/child OMS
- TWAP, VWAP, POV, Market, and Iceberg execution schedules
- Smart order routing across multiple venues
- Fee- and latency-aware route optimization
- Execution simulation with partial fills, rejections, latency, and fees
- SQLite-backed routed execution history
- FIX 4.4 codec, session state machine, sequence recovery, resend, and gap fill
- FIX-to-OMS order gateway
- Live Coinbase, Binance, and Kraken L2 market feeds
- Venue health and synchronization monitoring
- Market-data recording and deterministic replay
- Live/replay WebSocket streaming
- React operations and trading dashboard
- End-to-end verification and benchmark tooling

---

## Architecture

```text
                   ┌─────────────────────┐
                   │    Exchange Feeds   │
                   │ Coinbase / Binance  │
                   │       Kraken        │
                   └──────────┬──────────┘
                              │
                              ▼
                  ┌──────────────────────┐
                  │ Market Normalization │
                  │ L2 snapshots/updates │
                  └──────────┬───────────┘
                             │
              ┌──────────────┼───────────────┐
              ▼              ▼               ▼
       Venue Health    Market Recorder   Consolidated Book
              │              │               │
              │              ▼               ▼
              │         .mmdata files   Smart Order Router
              │                              │
              └──────────────┬───────────────┘
                             ▼
                    WebSocket Market API
                             │
                             ▼
                    React Trading Terminal

FIX Client
    │
    ▼
FIX 4.4 Session
    │
    ├── Sequence validation
    ├── Heartbeat / TestRequest
    ├── Resend / Gap Fill
    └── SQLite message persistence
    │
    ▼
FIX Order Gateway
    │
    ▼
OMS
    │
    ├── Parent Orders
    ├── Child Orders
    └── Execution Reports
    │
    ▼
Matching / Execution / Risk
Matching Engine

The matching engine uses price-time priority and supports:

Limit orders
Market orders
IOC
FOK
Post-only
Cancel
Replace/amend
Partial fills
Multi-symbol isolation

The order book is implemented as an in-memory data structure optimized for deterministic execution and low latency.

Example behavior:

SELL 50 @ 10000
BUY MARKET 20

Result:
Trade: 20 @ 10000
Resting sell quantity: 30
Deterministic Event Replay

MiniMatch writes binary event logs that can reconstruct exchange state exactly.

Example:

./build/minimatch_replay minimatch.events

Running the same replay multiple times produces the same state hash.

Example output:

events=2
input_sequence=2
symbols=1
active=1
state_hash=7633056132652409449

This is useful for:

debugging
regression testing
post-mortem analysis
deterministic state reconstruction
Order Management System

The OMS models a parent-child execution lifecycle.

Parent Order
    │
    ├── Child Order 1
    ├── Child Order 2
    └── Child Order N
            │
            ▼
      Execution Reports
            │
            ▼
           Fills

Supported parent strategies include:

Market
TWAP
VWAP
POV
Iceberg

The OMS tracks:

requested quantity
filled quantity
remaining quantity
child order status
execution reports
fees
venue
timestamps
Execution Algorithms

MiniMatch supports multiple execution schedules.

TWAP

Splits quantity across time intervals.

VWAP

Allocates quantity according to a supplied volume profile.

POV

Targets a percentage of expected market volume.

Iceberg

Limits displayed child quantity while progressively executing the parent order.

Market

Executes immediately against available liquidity.

Smart Order Router

The router evaluates multi-venue liquidity using:

execution price
available quantity
taker fees
latency
latency cost
slippage constraints
venue count limits
limit prices
all-or-none semantics

Example flow:

RouteRequest
    │
    ▼
Coinbase liquidity
Kraken liquidity
Binance liquidity
    │
    ▼
Effective price calculation
    │
    ▼
RoutePlan
    │
    ├── Venue A: quantity X
    ├── Venue B: quantity Y
    └── Venue C: quantity Z
Execution Simulation

Routed executions can be simulated with configurable:

fill ratio
rejection probability
base latency
latency jitter
random seed

The result tracks:

requested quantity
filled quantity
remaining quantity
average fill price
total notional
fees
total latency
per-child execution status

Execution history is persisted to SQLite and survives API restarts.

FIX 4.4

MiniMatch includes a functional FIX 4.4 stack.

Features include:

FIX encoding and parsing
checksum validation
body-length validation
Logon
Logout
Heartbeat
TestRequest
ResendRequest
SequenceReset / GapFill
message sequencing
persistent session state
persistent inbound/outbound messages
NewOrderSingle
OrderCancelRequest
ExecutionReport
FIX-to-OMS routing

Example session:

IN  Logon
OUT Logon

IN  NewOrderSingle
OUT ExecutionReport ACK

IN  OrderCancelRequest
OUT ExecutionReport CANCEL
Live Market Data

MiniMatch connects to live crypto exchange feeds and normalizes L2 data from:

Coinbase
Binance
Kraken

The live market gateway maintains:

per-venue books
consolidated BBO
midpoint
spread
venue sequence state
smart routing previews

The dashboard stream is exposed through WebSocket.

Venue Health

Each venue is continuously classified as:

UNKNOWN
HEALTHY
DELAYED
STALE
DISCONNECTED

Metrics include:

synchronization state
quote age
message count
snapshot count
update count
messages per second
reconnect count
rejected updates
sequence gaps
checksum errors

A venue is routable only when it is synchronized and sufficiently fresh.

Market Recording and Replay

Normalized market data can be recorded during live ingestion:

./build/minimatch_live_market_ws \
  8090 \
  BTC-USD \
  --record data/recordings/btcusd_live.mmdata

Replay:

./build/minimatch_market_replay_ws \
  8091 \
  data/recordings/btcusd_live.mmdata \
  BTC-USD \
  1.0

Replay supports:

play
pause
restart
speed control
seek by record
seek by timestamp
deterministic state checksums

The same frontend can consume either LIVE or REPLAY mode.

Frontend

The React frontend includes pages for:

Overview
Markets
Trading
Execution
OMS
Risk
FIX
Replay
Backtest
Analytics
System
Operations

The frontend supports a shared market-data provider with interchangeable:

LIVE
or
REPLAY

sources.

Benchmarks

Example local results on an Apple Silicon development machine:

1,000,000 orders
8 symbols
~6.59M generated orders/sec

Latency benchmark:

1,000,000 orders

p50   ~42 ns
p95   ~125 ns
p99   ~167 ns
p999  ~833 ns

Arena pipeline benchmark:

200,000 steps
200,000 processed
0 dropped
156,892 trades
456,873 reports

Results vary by hardware and build configuration.

For reproducibility, see:

benchmark_results/
Testing

MiniMatch includes extensive GoogleTest coverage across:

matching
order types
cancel/replace
replay
risk
OMS
execution schedules
router
multi-venue market data
Coinbase normalization
Binance normalization
Kraken normalization
backtesting
FIX codec
FIX session
FIX store
FIX gateway
execution persistence

Example:

ctest \
  --test-dir build \
  --output-on-failure

FIX-only tests:

./build/minimatch_tests \
  --gtest_filter='Fix*'
End-to-End Verification

Run:

./scripts/verify_e2e.sh

The script verifies:

HTTP API
Matching engine
Submit/cancel lifecycle
Order book
Historical backtest
OMS
Smart order routing
Execution simulation
SQLite execution persistence
FIX observability
Deterministic replay
Core test suite
One-Command Startup

Configure, build, test, and launch:

./scripts/dev_start.sh

Launch without rebuilding:

./scripts/dev_up.sh

This starts:

HTTP API
FIX gateway
Live market gateway
Replay server
React frontend

Logs are written under:

logs/
Manual Build
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build \
  -j"$(sysctl -n hw.ncpu)"

ctest \
  --test-dir build \
  --output-on-failure

Frontend:

cd frontend
npm install
npm run build
npm run dev
Example API Requests

Submit an order:

curl -X POST \
  http://127.0.0.1:8081/api/orders \
  -H 'Content-Type: application/json' \
  -d '{
    "orderId": 1001,
    "clientId": 10,
    "side": "buy",
    "type": "limit",
    "price": 10000,
    "quantity": 100,
    "symbol": 1
  }'

Run a TWAP backtest:

curl -X POST \
  http://127.0.0.1:8081/api/backtest \
  -H 'Content-Type: application/json' \
  -d '{
    "symbol": "btcusd",
    "side": "buy",
    "quantity": 2,
    "algorithm": "TWAP",
    "slices": 2,
    "durationSeconds": 2,
    "takerFeeBps": 5
  }'

Preview smart routing:

curl -X POST \
  http://127.0.0.1:8081/api/router/preview \
  -H 'Content-Type: application/json' \
  -d '{
    "symbol": "btcusd",
    "side": "buy",
    "quantity": 0.2,
    "maxSlippageBps": 100,
    "maxVenueCount": 3
  }'
Project Goals

MiniMatch was built to explore the systems problems behind modern trading infrastructure:

deterministic matching
low-latency data structures
real-time market-data processing
execution quality
smart routing
protocol-level reliability
risk management
observability
replayability

It is intended as an educational and engineering project, not a production trading system.

Tech Stack
Backend
C++20
Boost.Asio
Boost.Beast
OpenSSL
SQLite
GoogleTest
Google Benchmark
CMake
Frontend
React
TypeScript
Vite
TanStack Query
Infrastructure
WebSockets
FIX 4.4
Binary event logs
SQLite persistence
Live exchange APIs
License

This project is intended for educational and portfolio use.

---

## Screenshots

### Live Markets

![Live Markets](docs/screenshots/markets.png)

### Smart Order Routing & Execution

![Execution](docs/screenshots/execution.png)

### FIX 4.4 Session Monitor

![FIX](docs/screenshots/fix.png)

### Deterministic Replay

![Replay](docs/screenshots/replay.png)

### Venue Health & Operations

![Operations](docs/screenshots/operations.png)
