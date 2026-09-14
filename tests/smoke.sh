#!/bin/sh
set -eu

PORT=${NETPROBE_TEST_PORT:-19000}
SERVER_LOG=$(mktemp)
CLIENT_LOG=$(mktemp)
SERVER_PID=""

cleanup() {
    if [ -n "$SERVER_PID" ]; then
        kill "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
    fi
    rm -f "$SERVER_LOG" "$CLIENT_LOG"
}

trap cleanup EXIT INT TERM

./netprobe server --port "$PORT" >"$SERVER_LOG" 2>&1 &
SERVER_PID=$!
sleep 1

./netprobe client 127.0.0.1 --port "$PORT" --duration 0.2 --buffer-size 4096 >"$CLIENT_LOG" 2>&1

grep -q '^Latency$' "$CLIENT_LOG"
grep -q '^Throughput$' "$CLIENT_LOG"
grep -q '^Transferred$' "$CLIENT_LOG"

printf 'smoke test passed\n'
