/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include "defs.h"
#include "structs.h"


void      makev2cv(mflag, nvtxs, v2cv)
/* Construct mapping from original graph vtxs to coarsened graph vtxs. */
int      *mflag;		/* flag indicating vtx selected or not */
int       nvtxs;		/* number of vtxs in original graph */
int      *v2cv;			/* mapping from vtxs to coarsened vtxs */
{
    int       i, j;		/* loop counters */

    j = 1;
    for (i = 1; i <= nvtxs; i++) {
	if (mflag[i] == 0 || mflag[i] > i)
	    v2cv[i] = j++;
	else
	    v2cv[i] = v2cv[mflag[i]];
    }
}
