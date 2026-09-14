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

The client runs a latency test first, then opens a new connection for the throughput test.

## Tests

Run the latency statistics unit test:

```sh
make test
```

Run the local client/server smoke test after building:

```sh
sh tests/smoke.sh
```
