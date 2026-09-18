# NetProbe

NetProbe is a lightweight Linux network benchmarking tool written in C. The project focuses on TCP sockets, concurrency, and practical network performance measurement without external frameworks.

The design uses a simple client/server model. A NetProbe server accepts benchmark connections, while the client runs latency and throughput tests against it.

## Current features

- TCP server and client modes
- configurable port
- round-trip latency measurement
- min, average, max, p95, and p99 latency statistics
- sustained single-stream throughput measurement
- configurable buffer size and test duration
- clear terminal output and basic connection error handling

Parallel TCP streams with pthreads are still planned.

## Platform

NetProbe targets Linux and uses POSIX sockets, pthreads, `clock_gettime()`, and standard system APIs.

## Build

```sh
make
```

## Usage

Start a server:

```sh
./netprobe server --port 9000
```

Run a client against it:

```sh
./netprobe client 127.0.0.1 --port 9000 --duration 10 --buffer-size 65536
```

The default port is `9000`, the default throughput duration is 10 seconds, and the default buffer size is 64 KiB. The client runs a 20-sample latency test first, then opens a new connection for the throughput test.

## Tests

Build the project and run the full test suite:

```sh
make
make test
```

`make test` runs the latency statistics unit tests, command-line validation tests, and a local client/server smoke test. The same test target runs in GitHub Actions on pushes and pull requests.
