/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include "defs.h"
#include "structs.h"

void      sorthog(vec, n, solist, ngood)
double   *vec;			/* vector to be orthogonalized */
int       n;			/* length of the columns of orth */
struct orthlink **solist;	/* set of vecs to orth. against */
int       ngood;		/* number of vecs in solist */
{
    double    alpha;
    double   *dir;
    double    dot();
    void      scadd();
    int       i;

    for (i = 1; i <= ngood; i++) {
	dir = (solist[i])->vec;
	alpha = -dot(vec, 1, n, dir) / dot(dir, 1, n, dir);
	scadd(vec, 1, n, alpha, dir);
    }
}

void      sorthog_float(vec, n, solist, ngood)
float    *vec;			/* vector to be orthogonalized */
int       n;			/* length of the columns of orth */
struct orthlink_float **solist;	/* set of vecs to orth. against */
int       ngood;		/* number of vecs in solist */
{
    float     alpha;
    float    *dir;
    double    dot_float();
    void      scadd_float();
    int       i;

    for (i = 1; i <= ngood; i++) {
	dir = (solist[i])->vec;
	alpha = -dot_float(vec, 1, n, dir) / dot_float(dir, 1, n, dir);
	scadd_float(vec, 1, n, alpha, dir);
    }
}
