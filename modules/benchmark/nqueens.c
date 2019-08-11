/*
 * N-Queens Problem Solver
 * Found somewhere on the Internet; can't remember where. Possibly Wikipedia.
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "hardinfo.h"
#include "benchmark.h"

/* if anything changes in this block, increment revision */
#define BENCH_REVISION 0
#define QUEENS 11

int row[QUEENS];

bool safe(int x, int y) {
    int i;
    for (i = 1; i <= y; i++)
        if (row[y - i] == x || row[y - i] == x - i || row[y - i] == x + i)
            return false;
    return true;
}

int nqueens(int y) {
    int x;

    for (x = 0; x < QUEENS; x++) {
        if (safe((row[y - 1] = x), y - 1)) {
            if (y < QUEENS) {
                nqueens(y + 1);
            } else {
                break;
            }
        }
    }

    return 0;
}

static gpointer nqueens_for(unsigned int start, unsigned int end, void *data, gint thread_number)
{
    unsigned int i;

    for (i = start; i <= end; i++) {
        nqueens(0);
    }

    return NULL;
}

void
benchmark_nqueens(void)
{
    bench_value r = EMPTY_BENCH_VALUE;

    shell_view_set_enabled(FALSE);
    shell_status_update("Running N-Queens benchmark...");

    r = benchmark_parallel_for(0, 0, 10, nqueens_for, NULL);
    r.result = r.elapsed_time;
    r.revision = BENCH_REVISION;
    snprintf(r.extra, 255, "q:%d", QUEENS);

    bench_results[BENCHMARK_NQUEENS] = r;
}


