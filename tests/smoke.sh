#!/bin/sh
set -eu

PORT=${NETPROBE_TEST_PORT:-19000}
SERVER_LOG=$(mktemp)
CLIENT_LOG=$(mktemp)
SECOND_CLIENT_LOG=$(mktemp)
SERVER_PID=""

cleanup() {
    if [ -n "$SERVER_PID" ]; then
        kill "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
    fi
    rm -f "$SERVER_LOG" "$CLIENT_LOG" "$SECOND_CLIENT_LOG"
}

trap cleanup EXIT INT TERM

./netprobe server --port "$PORT" >"$SERVER_LOG" 2>&1 &
SERVER_PID=$!
sleep 1

./netprobe client 127.0.0.1 --port "$PORT" --duration 0.2 --buffer-size 4096 >"$CLIENT_LOG" 2>&1

grep -q '^Latency$' "$CLIENT_LOG"
grep -Eq '^  min  [0-9]+\.[0-9]{3} ms$' "$CLIENT_LOG"
grep -Eq '^  avg  [0-9]+\.[0-9]{3} ms$' "$CLIENT_LOG"
grep -Eq '^  p95  [0-9]+\.[0-9]{3} ms$' "$CLIENT_LOG"
grep -Eq '^  p99  [0-9]+\.[0-9]{3} ms$' "$CLIENT_LOG"
grep -Eq '^  max  [0-9]+\.[0-9]{3} ms$' "$CLIENT_LOG"
grep -q '^Throughput$' "$CLIENT_LOG"
grep -Eq '^  [0-9]+\.[0-9]{2} Mbps$' "$CLIENT_LOG"
grep -q '^Transferred$' "$CLIENT_LOG"
grep -Eq '^  [0-9]+\.[0-9]{2} MB$' "$CLIENT_LOG"

./netprobe client 127.0.0.1 --port "$PORT" --duration 0.1 --buffer-size 1024 >"$SECOND_CLIENT_LOG" 2>&1
grep -q '^Latency$' "$SECOND_CLIENT_LOG"
grep -q '^Throughput$' "$SECOND_CLIENT_LOG"

kill -0 "$SERVER_PID"

printf 'smoke test passed\n'
