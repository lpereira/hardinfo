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
#define BENCH_REVISION 2
#define QUEENS 6
#define CRUNCH_TIME 5

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

static gpointer nqueens_for(void *data, gint thread_number)
{
    nqueens(0);

    return NULL;
}

void
benchmark_nqueens(void)
{
    bench_value r = EMPTY_BENCH_VALUE;

    shell_view_set_enabled(FALSE);
    shell_status_update("Running N-Queens benchmark...");

    r = benchmark_crunch_for(CRUNCH_TIME, 0, nqueens_for, NULL);

    r.revision = BENCH_REVISION;
    snprintf(r.extra, 255, "q:%d", QUEENS);

    r.result /= 25;
    
    bench_results[BENCHMARK_NQUEENS] = r;
}


