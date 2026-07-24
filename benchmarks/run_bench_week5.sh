#!/bin/bash

RESULTS="benchmarks/results/week5.md"
mkdir -p benchmarks/results

KVSTORE_BIN="./build/kvstore"
KVSTORE_PORT=6380
REDIS_PORT=6379
OPS=100000

echo "# Week 5 Benchmark Results" > "$RESULTS"
echo "" >> "$RESULTS"
echo "redis-benchmark -t set,get -n $OPS" >> "$RESULTS"
echo "" >> "$RESULTS"
echo "| Server | Command | Clients | ops/sec | p50 latency (ms) | p99 latency (ms) |" >> "$RESULTS"
echo "|--------|---------|---------|---------|-----------------|-----------------|" >> "$RESULTS"

# --- KVStore ---
echo "Starting kvstore server..."
fuser -k ${KVSTORE_PORT}/tcp 2>/dev/null || true
sleep 1
$KVSTORE_BIN &
KVSTORE_PID=$!
sleep 1

for clients in 10 50; do
    for cmd in set get; do
        OUTPUT=$(redis-benchmark -p $KVSTORE_PORT -t $cmd -n $OPS -c $clients --csv 2>/dev/null | tail -1)
        OPS_SEC=$(echo "$OUTPUT" | cut -d',' -f2 | tr -d '"')
        P50=$(echo "$OUTPUT" | cut -d',' -f3 | tr -d '"')
        P99=$(echo "$OUTPUT" | cut -d',' -f7 | tr -d '"')
        echo "| kvstore | $cmd | $clients | $OPS_SEC | $P50 | $P99 |" >> "$RESULTS"
    done
done

echo "Stopping kvstore server..."
kill $KVSTORE_PID
wait $KVSTORE_PID 2>/dev/null
sleep 1

# --- Real Redis ---
echo "Starting Redis server..."
redis-server --daemonize yes --port $REDIS_PORT 2>/dev/null
sleep 1

for clients in 10 50; do
    for cmd in set get; do
        OUTPUT=$(redis-benchmark -p $REDIS_PORT -t $cmd -n $OPS -c $clients --csv 2>/dev/null | tail -1)
        OPS_SEC=$(echo "$OUTPUT" | cut -d',' -f2 | tr -d '"')
        P50=$(echo "$OUTPUT" | cut -d',' -f3 | tr -d '"')
        P99=$(echo "$OUTPUT" | cut -d',' -f7 | tr -d '"')
        echo "| redis | $cmd | $clients | $OPS_SEC | $P50 | $P99 |" >> "$RESULTS"
    done
done

echo "Stopping Redis server..."
redis-cli -p $REDIS_PORT shutdown nosave 2>/dev/null

echo "Results written to $RESULTS"
