# MiniMatch(a)

<p align="center">
  <img src="frontend/public/minimatch-logo.png" alt="MiniMatch(a) logo" width="96" />
</p>

<p align="center">
  <strong>A deterministic trading systems lab for matching, routing, risk, market data, replay, execution analytics, and AI-assisted system exploration.</strong>
</p>

<p align="center">
  <a href="https://minimatch-six.vercel.app">Live Demo</a>
  ·
  <a href="https://minimatch-api.onrender.com/api/health">API Health</a>
</p>

---

## Overview

**MiniMatch(a)** is a full-stack electronic trading systems project built to explore how the major components of an exchange and execution platform fit together.

It started as a deterministic C++ matching engine and expanded into a broader trading infrastructure environment with:

- Price-time-priority matching
- Limit, market, IOC, FOK, and post-only orders
- Multi-venue market data
- Smart order routing
- Order management
- Execution simulation
- Portfolio and client risk
- FIX-style connectivity
- Drop copy
- Deterministic replay
- Backtesting
- Quantitative analytics
- Operational monitoring
- An integrated AI assistant named **Matcha**

The project focuses on the engineering problems behind stateful trading systems: determinism, latency, routing, risk, observability, replay, persistence, and failure isolation.

---

## Live Demo

### Frontend

```text
https://minimatch-six.vercel.app
```

### API

```text
https://minimatch-api.onrender.com
```

Health check:

```bash
curl https://minimatch-api.onrender.com/api/health
```

---

## Architecture

```text
                         ┌─────────────────────┐
                         │    MiniMatch(a)     │
                         │   React Frontend    │
                         └──────────┬──────────┘
                                    │
                             REST / WebSocket
                                    │
                         ┌──────────▼──────────┐
                         │      C++ API        │
                         │   Control Plane     │
                         └──────────┬──────────┘
                                    │
             ┌──────────────────────┼──────────────────────┐
             │                      │                      │
             ▼                      ▼                      ▼
     ┌───────────────┐      ┌───────────────┐      ┌───────────────┐
     │ Matching      │      │ Smart Order   │      │ Risk Engine   │
     │ Engine        │      │ Router        │      │               │
     └───────┬───────┘      └───────┬───────┘      └───────┬───────┘
             │                      │                      │
             ▼                      ▼                      ▼
     ┌───────────────┐      ┌───────────────┐      ┌───────────────┐
     │ Order Book    │      │ Venue Quotes  │      │ Positions /   │
     │ + Executions  │      │ + Simulation  │      │ Portfolio     │
     └───────┬───────┘      └───────────────┘      └───────────────┘
             │
             ▼
     ┌─────────────────────────────────────────────────────────────┐
     │ OMS · FIX · Drop Copy · Persistence · Replay · Backtesting │
     └─────────────────────────────────────────────────────────────┘
```

---

## Core Systems

### Matching Engine

The C++ matching engine implements deterministic **price-time priority**.

Supported order behavior includes:

- Limit orders
- Market orders
- Immediate-or-Cancel
- Fill-or-Kill
- Post-only
- Partial fills
- Cancellations
- Amendments
- FIFO within a price level
- Best-price priority
- Multi-symbol isolation

The same ordered input stream is designed to produce the same resulting state, making deterministic replay possible.

---

### Smart Order Router

The router evaluates simulated venue liquidity using:

- Price
- Available quantity
- Taker fees
- Venue latency
- Estimated latency cost
- Maximum slippage
- Maximum venue count
- All-or-none constraints

Example:

```bash
curl \
  -X POST \
  https://minimatch-api.onrender.com/api/router/preview \
  -H 'Content-Type: application/json' \
  -d '{
    "symbol": "btcusd",
    "side": "buy",
    "quantity": 0.1,
    "maxSlippageBps": 100,
    "maxVenueCount": 3,
    "allOrNone": false
  }'
```

---

### Order Management System

The OMS manages execution state above individual exchange orders.

It tracks:

- Parent orders
- Child orders
- Algorithmic execution
- Fills
- Remaining quantity
- Execution status
- Fill notional
- Reconciliation state

---

### Risk Engine

MiniMatch(a) models both client-level and system-level protection mechanisms.

Risk controls include:

- Position limits
- Portfolio limits
- Client exposure
- Self-trade prevention
- Price bands
- Circuit breakers
- Symbol halts
- Global trading halt
- Daily-loss protection

---

### Market Data

The market-data subsystem exposes consolidated state across simulated or live venues.

The dashboard includes:

- Best bid and ask
- Midpoint
- Spread
- Order-book depth
- Venue liquidity
- Trade tape
- Message rates
- Sequence integrity
- Venue synchronization
- Pipeline latency
- BBO history

---

### FIX and Drop Copy

MiniMatch(a) includes FIX-oriented infrastructure for exploring electronic trading connectivity.

The system exposes:

- Session state
- Inbound sequence numbers
- Outbound sequence numbers
- Execution reports
- Rejects
- Resend activity
- Sequence resets
- Message history
- Drop-copy records

---

### Deterministic Replay

Recorded events can be replayed to reconstruct historical system state.

Replay supports:

- Restart
- Pause
- Resume
- Seek
- Playback speed control
- Intermediate-state inspection

Replay is used as both a debugging mechanism and a foundation for historical analysis.

---

### Backtesting

The backtesting environment evaluates execution algorithms against recorded market data.

It includes metrics such as:

- Arrival-price slippage
- VWAP comparison
- Filled quantity
- Execution notional
- Fill progression
- Historical execution behavior

---

### Analytics

The analytics layer includes research-oriented tooling for:

- Portfolio analysis
- Pairs trading diagnostics
- Statistical arbitrage
- Options pricing
- Execution quality
- Latency analysis

---

## Matcha

<p align="center">
  <img src="frontend/public/matcha-agent.png" alt="Matcha" width="80" />
</p>

**Matcha** is the AI assistant built into MiniMatch(a).

Matcha can use application context to help explain:

- Matching behavior
- Current market state
- Orders
- Executions
- OMS state
- Positions
- Portfolio risk
- System health
- Trading-system concepts

The goal is to make a complex trading system easier to inspect without hiding the underlying infrastructure.

---

## Dashboard

| Page | Purpose |
|---|---|
| **Overview** | High-level market and system state |
| **Markets** | Quotes, BBO, depth, liquidity, and trade tape |
| **Trading** | Direct order entry and order lifecycle management |
| **Execution** | Smart routing and execution simulation |
| **OMS** | Parent orders, child orders, and fills |
| **Risk** | Client, instrument, and portfolio protection |
| **FIX** | Protocol session and message inspection |
| **Replay** | Deterministic historical reconstruction |
| **Backtest** | Historical execution evaluation |
| **Analytics** | Quantitative research tools |
| **System** | Matching-engine internals and performance |
| **Operations** | Market-data infrastructure and operational health |
| **Journal** | Engineering decisions and lessons learned |

---

## Tech Stack

### Backend

```text
C++20
Boost.Asio
SQLite
CMake
Google Test
Google Benchmark
```

### Frontend

```text
React
TypeScript
Vite
TanStack Query
Recharts
Lucide
```

### Deployment

```text
Vercel
Render
```

---

## Repository Structure

```text
minimatch/
├── apps/
│   └── api.cpp
│
├── include/
│   └── minimatch/
│
├── src/
│   ├── matching
│   ├── routing
│   ├── OMS
│   ├── risk
│   ├── replay
│   ├── persistence
│   └── analytics
│
├── tests/
│
├── scripts/
│   ├── verify_e2e.sh
│   └── test_router_preview.sh
│
├── frontend/
│   ├── public/
│   │   ├── background.mp4
│   │   ├── minimatch-logo.png
│   │   ├── matcha-agent.png
│   │   └── favicon.png
│   │
│   └── src/
│       ├── api/
│       ├── components/
│       ├── context/
│       ├── hooks/
│       └── pages/
│
└── CMakeLists.txt
```

---

## Local Development

### Backend

```bash
git clone <YOUR_REPOSITORY_URL>
cd minimatch

cmake -S . -B build
cmake --build build -j
```

Run tests:

```bash
ctest --test-dir build --output-on-failure
```

---

### Frontend

```bash
cd frontend

npm install
npm run dev
```

Production build:

```bash
npm run build
```

---

## API Examples

### Health

```bash
curl http://127.0.0.1:8080/api/health
```

### System

```bash
curl http://127.0.0.1:8080/api/system
```

### Portfolio

```bash
curl http://127.0.0.1:8080/api/portfolio
```

### Portfolio Risk

```bash
curl http://127.0.0.1:8080/api/portfolio/risk
```

### Executions

```bash
curl http://127.0.0.1:8080/api/executions
```

### FIX Session

```bash
curl http://127.0.0.1:8080/api/fix/session
```

---

## Testing

The test suite covers behavior including:

- FIFO within a price level
- Best-price priority
- Market orders do not rest
- IOC remainder cancellation
- FOK behavior
- Post-only behavior
- Partial fills
- Cancellations
- Amendments
- Multi-symbol isolation
- Risk limits
- Deterministic replay
- Snapshot round trips
- Smart-routing behavior

Run:

```bash
ctest --test-dir build --output-on-failure
```

Router smoke test:

```bash
BASE_URL=http://127.0.0.1:8080 \
bash scripts/test_router_preview.sh
```

Against production:

```bash
BASE_URL=https://minimatch-api.onrender.com \
bash scripts/test_router_preview.sh
```

---

## Engineering Principles

### Determinism before optimization

Performance matters, but reproducibility makes stateful systems debuggable and trustworthy.

### Explicit system boundaries

Matching, market data, OMS, routing, risk, persistence, and analytics have different responsibilities and failure modes.

### Observability is part of the product

Latency, synchronization, sequence integrity, reconciliation, and recovery state should be visible rather than hidden inside logs.

### Risk exists throughout the lifecycle

Risk is not only a pre-trade check. Orders, fills, positions, portfolio state, and market conditions all affect protection decisions.

### Performance should remain measurable

Optimization is most useful when latency and throughput can be compared against a reproducible baseline.

---

## Engineering Journal

The Journal documents major stages of the project:

1. Determinism Before Performance
2. Building the Order Book
3. Real-Time Market Data
4. Smart Order Routing
5. Risk as a System
6. Deterministic Replay
7. Execution Algorithms and Backtesting
8. FIX and Post-Trade Infrastructure
9. Observability and Operations
10. Architecture After Feature Growth

---

## Disclaimer

MiniMatch(a) is an educational and engineering project.

It is not a production exchange, broker, investment platform, or financial advisory service. Simulated market behavior and execution results should not be used for real trading decisions.

---

<p align="center">
  <img src="frontend/public/matcha-agent.png" alt="Matcha" width="55" />
</p>

<p align="center">
  <strong>MiniMatch(a)</strong><br />
  Matching systems, with a little matcha.
</p>
