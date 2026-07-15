# Week 2 Benchmark Results — Explanation

## Why does shared_mutex show higher p99 latency than mutex?

### shared_mutex is more complex internally

A regular `std::mutex` is a simple binary lock — one atomic operation to acquire, one to release.

`std::shared_mutex` has to maintain more state: how many readers currently hold the lock,
whether a writer is waiting, whether a writer holds it. Every `shared_lock` acquisition
involves atomic read-modify-write operations to update the reader count. That bookkeeping
has a cost per operation.

### shared_mutex only wins under specific conditions

It pays off only when:
- We have **many concurrent threads** all reading simultaneously, AND
- Reads are **significantly slower** than the lock overhead (e.g. reading from disk, doing heavy computation)

For an in-memory hash map lookup that completes in nanoseconds, the lock overhead itself
dominates. The "parallel reads" benefit is tiny compared to the bookkeeping cost.

### Our workload is low-contention

With 95% reads on a 10,000-key map, threads rarely collide on the same key at the same
moment. A plain `mutex` releases so fast that the next thread barely waits. `shared_mutex`
adds overhead for a contention problem that barely exists here.

### Rule of thumb

> `shared_mutex` is for protecting resources where the *work inside the lock* is expensive.
> For cheap in-memory operations, it usually loses to a plain `mutex`.

This is exactly why Week 3 moves to sharding — instead of a smarter lock, we reduce
contention by splitting the map so threads rarely compete for the same lock at all.
