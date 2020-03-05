/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include "structs.h"

void      orthogonalize(vec, n, orthlist)
double   *vec;			/* vector to be orthogonalized */
int       n;			/* length of the columns of orth */
struct orthlink *orthlist;	/* set of vectors to orthogonalize against */
{
    struct orthlink *curlnk;
    void      orthogvec();

    curlnk = orthlist;
    while (curlnk != NULL) {
	orthogvec(vec, 1, n, curlnk->vec);
	curlnk = curlnk->pntr;
    }
}
