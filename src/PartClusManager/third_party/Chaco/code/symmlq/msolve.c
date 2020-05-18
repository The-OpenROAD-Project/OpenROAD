/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include "structs.h"

int       msolve_(lnvtxs, x, y, dA, vwsqrt, work)
long     *lnvtxs;
double   *x;
double   *y;
double   *dA;
double   *vwsqrt;
double   *work;
{
    struct vtx_data **A;
    int       nvtxs;
    int       i;

    A = (struct vtx_data **) dA;
    nvtxs = (int) *lnvtxs;

    /* to placate alint */
    A = A;
    vwsqrt = vwsqrt;

    /* Offset arrays for our C numbering. */
    x -= 1;
    y -= 1;
    work -= 1;

    /* Just do a copy for now. */
    for (i = nvtxs; i; i--) {
	y[i] = x[i];
    }

    /* Restore arrays to Fortran numbering. */
    x -= 1;
    y -= 1;
    work -= 1;

    return (0);
}
