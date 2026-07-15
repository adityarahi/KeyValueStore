#!/bin/bash

RESULTS="benchmarks/results/week2.md"
mkdir -p benchmarks/results

echo "# Week 2 Benchmark Results" > "$RESULTS"
echo "" >> "$RESULTS"
echo "| Store | Threads | ops/sec | p99 latency (µs) |" >> "$RESULTS"
echo "|-------|---------|---------|-----------------|" >> "$RESULTS"

for store in mutex shared_mutex; do
    for threads in 1 2 4 8; do
        OUTPUT=$(./build/bench --threads "$threads" --ratio 95 --ops 1000000 --store "$store")
        OPS=$(echo "$OUTPUT" | grep "ops/sec" | awk '{print $2}')
        P99=$(echo "$OUTPUT" | grep "p99" | awk '{print $3}')
        echo "| $store | $threads | $OPS | $P99 |" >> "$RESULTS"
    done
done

echo "Results written to $RESULTS"
