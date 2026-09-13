#include <math.h>
#include <stdio.h>

#include "stats.h"

static int close_enough(double left, double right)
{
    return fabs(left - right) < 0.000001;
}

static int test_latency_stats(void)
{
    double samples[] = {4.0, 1.0, 5.0, 2.0, 3.0};
    latency_stats stats;

    if (calculate_latency_stats(samples, 5, &stats) != 0) {
        return -1;
    }

    if (!close_enough(stats.min_ms, 1.0) ||
        !close_enough(stats.avg_ms, 3.0) ||
        !close_enough(stats.p95_ms, 4.0) ||
        !close_enough(stats.p99_ms, 4.0) ||
        !close_enough(stats.max_ms, 5.0)) {
        return -1;
    }

    return 0;
}

static int test_invalid_input(void)
{
    double sample = 1.0;
    latency_stats stats;

    if (calculate_latency_stats(NULL, 1, &stats) == 0) {
        return -1;
    }

    if (calculate_latency_stats(&sample, 0, &stats) == 0) {
        return -1;
    }

    if (calculate_latency_stats(&sample, 1, NULL) == 0) {
        return -1;
    }

    return 0;
}

int main(void)
{
    if (test_latency_stats() != 0) {
        fprintf(stderr, "latency statistics test failed\n");
        return 1;
    }

    if (test_invalid_input() != 0) {
        fprintf(stderr, "invalid input test failed\n");
        return 1;
    }

    printf("stats tests passed\n");
    return 0;
}
