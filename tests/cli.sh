#!/bin/sh
set -eu

expect_failure() {
    description=$1
    shift

    if "$@" >/dev/null 2>&1; then
        echo "expected failure: $description" >&2
        exit 1
    fi
}

expect_failure "missing mode" ./netprobe
expect_failure "unknown mode" ./netprobe probe
expect_failure "client without host" ./netprobe client
expect_failure "port below range" ./netprobe server --port 0
expect_failure "port above range" ./netprobe server --port 65536
expect_failure "non-numeric duration" ./netprobe client 127.0.0.1 --duration nope
expect_failure "zero duration" ./netprobe client 127.0.0.1 --duration 0
expect_failure "zero buffer size" ./netprobe client 127.0.0.1 --buffer-size 0
expect_failure "server-only client option" ./netprobe server --duration 1
expect_failure "unknown option" ./netprobe server --bogus

echo "cli tests passed"
