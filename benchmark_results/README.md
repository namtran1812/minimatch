# MiniMatch Benchmark Summary

## Environment

See `environment.txt` for the full machine and compiler configuration.

## Matching latency benchmark

Workload: 1,000,000 deterministic synthetic orders per run  
Runs: 5

| Metric | Result |
|---|---:|
| Mean throughput | 12.08M orders/sec |
| Minimum throughput | 11.41M orders/sec |
| Maximum throughput | 12.59M orders/sec |
| Median p50 latency | 42 ns |
| Median p95 latency | 125 ns |
| Median p99 latency | 208 ns |
| Median p99.9 latency | 1084 ns |

All five runs produced the same final state hash:

`16864509674985480077`

## Multi-symbol load generator

Workload: 1,000,000 deterministic synthetic orders over 8 symbols  
Runs: 5

| Metric | Result |
|---|---:|
| Mean throughput | 7.50M orders/sec |
| Minimum throughput | 6.32M orders/sec |
| Maximum throughput | 8.09M orders/sec |

All five runs produced the same final state hash:

`4496917943067555212`

## Notes

These are local synthetic Release-build results on the machine described in
`environment.txt`. They are not exchange round-trip latency or production
network latency.
