# NetProbe

NetProbe is a lightweight Linux network benchmarking tool written in C. The project focuses on TCP sockets, concurrency, and practical network performance measurement without external frameworks.

The design uses a simple client/server model. A NetProbe server accepts benchmark connections, while the client runs latency and throughput tests against it.

## Planned MVP

- TCP server and client modes
- configurable host and port
- round-trip latency measurement
- min, average, max, p95, and p99 latency statistics
- sustained throughput measurement
- configurable buffer size and test duration
- multiple parallel TCP connections with pthreads
- clear terminal output and graceful error handling

## Platform

NetProbe targets Linux and uses POSIX sockets, pthreads, `clock_gettime()`, and standard system APIs.

## Build

```sh
make
```

Implementation and usage details will be documented as the project develops.
