/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>


/* Scales vector by diagonal matrix (passed as vector) over range. */
void      scale_diag(vec, beg, end, diag)
double   *vec;			/* the vector to scale */
int       beg, end;		/* specify the range to norm over */
double   *diag;			/* vector to scale by */
{
    int       i;

    if (diag != NULL) {
	vec = vec + beg;
	diag = diag + beg;
	for (i = end - beg + 1; i; i--) {
	    *vec++ *= *diag++;
	}
    }
    /* otherwise return vec unchanged */
}

/* Scales vector by diagonal matrix (passed as vector) over range. */
void      scale_diag_float(vec, beg, end, diag)
float    *vec;			/* the vector to scale */
int       beg, end;		/* specify the range to norm over */
float    *diag;			/* vector to scale by */
{

    int       i;

    if (diag != NULL) {
	vec = vec + beg;
	diag = diag + beg;
	for (i = end - beg + 1; i; i--) {
	    *vec++ *= *diag++;
	}
    }
}
