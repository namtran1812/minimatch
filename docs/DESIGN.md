# MiniMatch v2 Design

## Hot path

Each symbol owns one `OrderBook`. A book is mutated by one logical writer, eliminating locks from matching and preserving deterministic arrival order. Price levels use ordered maps; orders within each level use intrusive indices into a preallocated node array, avoiding per-order list allocation.

## Priority

1. Best executable price.
2. FIFO among resting orders at that price.
3. Trades execute at the resting order price.
4. Same-price quantity reductions retain priority.
5. Price changes or quantity increases remove and reinsert the order, losing priority.

## Order semantics

- **Limit:** crosses eligible liquidity, then rests remaining quantity.
- **Market:** crosses all available liquidity and expires any remainder.
- **IOC:** crosses within its limit and expires any remainder.
- **FOK:** scans executable depth first and either fills completely or rejects without book mutation.
- **Post-only:** rejects if it would immediately trade.

## Determinism and recovery

The input journal uses contiguous event sequence numbers and a payload checksum. Replay applies the exact command stream to fresh state. Snapshots serialize resting orders in deterministic price/FIFO order together with per-book sequence and hash; loading verifies the reconstructed hash.

## Measurement

`minimatch_latency` pre-generates inputs so RNG cost is excluded, performs a warm-up, records per-submit nanoseconds, sorts samples, and reports p50/p95/p99/p99.9/max. This is useful for regression tracking, though Linux `perf`, CPU pinning, frequency controls, and repeated runs are still required for stronger performance claims.
