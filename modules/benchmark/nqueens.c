/*
 * N-Queens Problem Solver
 * Found somewhere on the Internet; can't remember where. Possibly Wikipedia.
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define QUEENS 11

int row[QUEENS];

bool safe(int x, int y)
{
    int i;
    for (i = 1; i <= y; i++)
	if (row[y - i] == x || row[y - i] == x - i || row[y - i] == x + i)
	    return false;
    return true;
}

int nqueens(int y)
{
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
