/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	<math.h>
#include "defs.h"

/* Check sturmcnt */
void      cksturmcnt(vec, beg, end, x1, x2, x1ck, x2ck, numck)
double   *vec, x1, x2;
int       beg, end;
int      *x1ck, *x2ck, *numck;
{

    int i, count;

    count = 0;
    for (i = beg; i <= end; i++) {
        if (vec[i] > x1) count += 1;
    }
    *x1ck = end - count;
    
    count = 0;
    for (i = beg; i <= end; i++) {
        if (vec[i] > x2) count += 1;
    }
    *x2ck = end - count;

    count = 0;
    for (i = beg; i <= end; i++) {
        if (vec[i] > x1 && vec[i] < x2) count += 1;
    }
    *numck = count;
}
