# Week 2 Benchmark Results

| Store | Threads | ops/sec | p99 latency (µs) |
|-------|---------|---------|-----------------|
| mutex | 1 | 1.22825e+06 | 1 |
| mutex | 2 | 1.40086e+06 | 12 |
| mutex | 4 | 976626 | 51 |
| mutex | 8 | 550456 | 109 |
| shared_mutex | 1 | 1.21051e+06 | 1 |
| shared_mutex | 2 | 1.14741e+06 | 31 |
| shared_mutex | 4 | 502120 | 104 |
| shared_mutex | 8 | 476585 | 357 |
