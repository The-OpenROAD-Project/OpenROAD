/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"structs.h"
#include	"defs.h"


int       make_sep_list(bspace, list_length, sets)
int      *bspace;		/* list of vtxs to be moved */
int       list_length;		/* current length of bspace */
short    *sets;			/* processor each vertex is assigned to */
{
    int       vtx;		/* vertex in list */
    int       i, k;		/* loop counters */

    /* Compress out the actual boundary vertices. */
    k = 0;
    for (i=0; i<list_length; i++) {
	vtx = bspace[i];
	if (vtx < 0) vtx = -vtx;
	if (sets[vtx] == 2) bspace[k++] = vtx;
    }

    bspace[k] = 0;
    return(k);
}
