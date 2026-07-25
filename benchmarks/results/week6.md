# Week 6 Benchmark Results

redis-benchmark -t set,get -n 100000

| Server | Command | Clients | ops/sec | p50 latency (ms) | p99 latency (ms) |
|--------|---------|---------|---------|-----------------|-----------------|
| kvstore | set | 10 | 31928.48 | 0.187 | 0.359 |
| kvstore | get | 10 | 33783.79 | 0.176 | 0.295 |
| kvstore | set | 50 | 40816.32 | 0.638 | 1.095 |
| kvstore | get | 50 | 34542.32 | 0.727 | 1.391 |
| redis | set | 10 | 127226.46 | 0.049 | 0.191 |
| redis | get | 10 | 128865.98 | 0.048 | 0.183 |
| redis | set | 50 | 132802.12 | 0.200 | 0.559 |
| redis | get | 50 | 129533.68 | 0.204 | 0.551 |
