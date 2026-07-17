# Week 3 Benchmark Results — Explanation

## Why does sharding outperform a single-lock store?

### The problem with a single lock

With `KVStoreMutex`, every operation — regardless of which key it touches — acquires the
same lock. A write on `"key_1"` blocks a read on `"key_9999"` even though they have
nothing to do with each other. As thread count grows, threads spend more time waiting than
doing actual work. This is why ops/sec drops and p99 latency spikes at 4 and 8 threads.

`KVStoreSharedMutex` has the same problem — parallel reads help in theory, but each write
still blocks all readers on the entire map. At high thread counts, write operations cause
long queues of waiting readers, making p99 worse than plain mutex.

### How sharding fixes this

`KVStoreSharded` divides the map into 16 buckets (shards), each with its own independent
lock. A key is routed to its shard via:

```
shard_index = std::hash(key) % 16
```

A write on `"key_1"` now only blocks operations on shard 0. Operations on shards 1–15
proceed in parallel, completely unaffected. Two threads working on keys in different shards
never contend at all.

### What the numbers show

| Store | Threads | ops/sec | p99 latency (µs) |
|-------|---------|---------|-----------------|
| mutex | 1 | ~709K | 3 |
| mutex | 8 | ~376K | 171 |
| sharded | 1 | ~752K | 3 |
| sharded | 8 | ~3.04M | 22 |

- mutex **loses throughput** as threads increase — contention dominates
- sharded **scales linearly** — doubling threads roughly doubles ops/sec
- p99 stays low under sharding because threads rarely queue behind each other

### Why alignas(64) matters

Each shard struct is aligned to 64 bytes — the size of a CPU cache line. Without this,
two shards could share a cache line. When thread A writes to shard 0 and thread B writes
to shard 1, the CPU would force both cores to synchronize that cache line on every write —
**false sharing** — even though the threads are working on completely separate data.
`alignas(64)` eliminates this by guaranteeing each shard lives on its own cache line.

### The key takeaway

Contention is reduced not by using a smarter lock, but by shrinking the region each lock
protects. Threads that would have serialized now run in parallel because they are unlikely
to hash to the same shard.
