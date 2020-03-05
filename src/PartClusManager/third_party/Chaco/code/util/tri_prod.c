/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>


double    tri_prod(v1, v2, v3, wsqrt, n)
double   *v1, *v2, *v3, *wsqrt;
int       n;

/* Form inner product of three vectors. */
{
    double    dot = 0;
    int       i;

    if (wsqrt == NULL) {	/* Unweighted case.  Use weights = 1. */
	for (i = 1; i <= n; i++) {
	    dot += v1[i] * v2[i] * v3[i];
	}
    }
    else {			/* Unweighted case. */
	for (i = 1; i <= n; i++) {
	    dot += v1[i] * v2[i] * v3[i] / wsqrt[i];
	}
    }
    return (dot);
}
