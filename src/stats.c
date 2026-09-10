#include <stdlib.h>

#include "stats.h"

static int compare_double(const void *left, const void *right)
{
    double a = *(const double *)left;
    double b = *(const double *)right;

    return (a > b) - (a < b);
}

static double percentile(const double *sorted, size_t count, double fraction)
{
    size_t index = (size_t)(fraction * (double)(count - 1));

    return sorted[index];
}

int calculate_latency_stats(const double *samples, size_t count, latency_stats *stats)
{
    double *sorted;
    double total = 0.0;
    size_t i;

    if (samples == NULL || stats == NULL || count == 0) {
        return -1;
    }

    sorted = malloc(count * sizeof(*sorted));
    if (sorted == NULL) {
        return -1;
    }

    for (i = 0; i < count; i++) {
        sorted[i] = samples[i];
        total += samples[i];
    }

    qsort(sorted, count, sizeof(*sorted), compare_double);

    stats->min_ms = sorted[0];
    stats->avg_ms = total / (double)count;
    stats->p95_ms = percentile(sorted, count, 0.95);
    stats->p99_ms = percentile(sorted, count, 0.99);
    stats->max_ms = sorted[count - 1];

    free(sorted);
    return 0;
}
