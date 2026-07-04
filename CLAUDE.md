# KV Store — Project Guide

A Redis-protocol-compatible concurrent key-value store in C++20, built as a 10-week learning project targeting HFT/infra-level systems engineering skills.

## Project Goals

- Learn concurrency primitives hands-on: mutexes → lock-free structures → memory reclamation
- Build something real: `redis-cli` talks to it, `redis-benchmark` measures it
- Produce resume-grade deliverables: benchmark tables, architecture docs, TSan-clean code

## Planned Architecture (evolves week by week)

```
Clients
  │  RESP protocol over TCP
  ▼
epoll event loop (multi-threaded, shared-nothing per Week 5)
  │
  ▼
Sharded hash map (16–64 buckets, each with its own lock)
  │
  ▼
Lock-free MPMC queue (Week 7+) for I/O → worker handoff
  │
  ▼
TTL reaper thread + LRU eviction (Week 6+)
```

## Week-by-Week Deliverables

| Week | Focus | Deliverable |
|------|-------|-------------|
| 1 | Foundations + single-threaded core | GET/SET/DEL on `std::unordered_map`, unit tests, CI |
| 2 | Thread-safe v1 + benchmark harness | Benchmark table: single lock vs `shared_mutex` at 1/2/4/8 threads |
| 3 | Sharded hash map | Near-linear read scaling; README explaining contention drop |
| 4 | epoll network server + RESP | `redis-cli -p 6380 SET foo bar` works |
| 5 | Threading model | `redis-benchmark` ops/sec vs real Redis |
| 6 | TTL/LRU + catch-up | Clean CI; feature-complete "mini-Redis" |
| 7 | Lock-free MPMC queue | Stress test passes under TSan (millions of ops) |
| 8 | Memory reclamation (epoch/hazard) | README: ABA problem + reclamation scheme explained |
| 9 | Optimization pass | `perf` profile; before/after benchmark table |
| 10 | Polish + publish | Architecture diagram, resume bullets, final numbers |

## Toolchain

- **Build**: CMake (minimum 3.20), C++20
- **Testing**: Google Test
- **Formatting**: clang-format (Google or LLVM style)
- **CI**: GitHub Actions (build + test on every push)
- **Sanitizers**: ASan (always), TSan (Week 7 onward, always)
- **Profiling**: `perf`, `perf stat`, `perf record`/`report`

## Key Design Decisions (record reasoning here as the project progresses)

- **Sharding strategy**: TBD — likely `std::hash(key) % N` with N a power of two for cheap modulo
- **Threading model**: TBD — comparing shared-nothing event loops vs I/O-thread + worker pool (Week 5)
- **Lock-free reclamation**: TBD — epoch-based vs hazard pointers (Week 8)

## Survival Rules

1. **Benchmark every week from Week 2** — a feature without a number is invisible on a resume
2. **TSan from Week 7, always** — lock-free bugs are silent without it; run the full test suite under TSan before every commit in weeks 7–9
3. **Never cut Week 4** — `redis-cli` talking to the server is the demo; weeks 7–9 can be scoped down, not week 4

## What to Measure

Every benchmark run should record:
- Thread counts tested: 1, 2, 4, 8
- Read/write ratio (e.g. 95/5, 80/20, 50/50)
- ops/sec (total throughput)
- p99 latency (µs)
- Comparison point (previous week's number, or real Redis)

Store raw results in `benchmarks/results/weekN.md`.

## Reference Reading

- *C++ Concurrency in Action* (Williams) — primary text; chapters 1–3 (Week 1), ch. 5 + 7 (Week 7, read twice)
- Beej's Guide to Network Programming — sockets primer (Week 4)
- `epoll(7)` man page — event loop (Week 4)
- RESP protocol spec — [redis.io/docs/reference/protocol-spec](https://redis.io/docs/reference/protocol-spec/)
- Dragonfly DB architecture — shared-nothing threading model reference (Week 5)

## Directory Layout (target)

```
kv_store/
├── CMakeLists.txt
├── CLAUDE.md
├── README.md
├── .clang-format
├── .github/workflows/ci.yml
├── src/
│   ├── main.cpp
│   ├── store/          # KV store implementations
│   ├── net/            # epoll event loop, RESP parser
│   └── concurrent/     # lock-free queue, epoch reclamation
├── tests/
│   ├── store_test.cpp
│   ├── concurrent_test.cpp
│   └── stress_test.cpp
└── benchmarks/
    ├── bench_main.cpp
    └── results/
        └── weekN.md    # raw numbers per week
```

## Commands

```bash
# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j$(nproc)

# Test
ctest --test-dir build --output-on-failure

# Test with TSan (Week 7+)
cmake -B build-tsan -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCMAKE_CXX_FLAGS="-fsanitize=thread" && \
cmake --build build-tsan -j$(nproc) && \
ctest --test-dir build-tsan --output-on-failure

# Benchmark
./build/benchmarks/bench --threads 8 --ratio 95 --ops 10000000

# Connect with redis-cli (Week 4+)
redis-cli -p 6380 SET foo bar
redis-benchmark -p 6380 -t set,get -n 1000000
```
