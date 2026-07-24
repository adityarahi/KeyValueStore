# Week 5 Benchmark Results

redis-benchmark -t set,get -n 100000

| Server | Command | Clients | ops/sec | p50 latency (ms) | p99 latency (ms) |
|--------|---------|---------|---------|-----------------|-----------------|
| kvstore | set | 10 | 25786.49 | 0.232 | 0.719 |
| kvstore | get | 10 | 28810.14 | 0.207 | 0.575 |
| kvstore | set | 50 | 35473.57 | 0.737 | 1.831 |
| kvstore | get | 50 | 35880.88 | 0.725 | 1.535 |
| redis | set | 10 | 110253.59 | 0.058 | 0.287 |
| redis | get | 10 | 112485.94 | 0.058 | 0.303 |
| redis | set | 50 | 117508.81 | 0.241 | 0.871 |
| redis | get | 50 | 106609.80 | 0.266 | 1.055 |
