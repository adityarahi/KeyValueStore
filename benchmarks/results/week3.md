# Week 3 Benchmark Results

| Store | Threads | ops/sec | p99 latency (µs) |
|-------|---------|---------|-----------------|
| mutex | 1 | 725273 | 3 |
| mutex | 2 | 643826 | 34 |
| mutex | 4 | 482018 | 105 |
| mutex | 8 | 379914 | 165 |
| shared_mutex | 1 | 936332 | 2 |
| shared_mutex | 2 | 721940 | 47 |
| shared_mutex | 4 | 341859 | 172 |
| shared_mutex | 8 | 324951 | 549 |
| sharded | 1 | 859145 | 2 |
| sharded | 2 | 1550988 | 6 |
| sharded | 4 | 2400190 | 10 |
| sharded | 8 | 3190380 | 21 |
