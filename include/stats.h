#ifndef NETPROBE_STATS_H
#define NETPROBE_STATS_H

#include <stddef.h>

typedef struct {
    double min_ms;
    double avg_ms;
    double p95_ms;
    double p99_ms;
    double max_ms;
} latency_stats;

int calculate_latency_stats(const double *samples, size_t count, latency_stats *stats);

#endif
