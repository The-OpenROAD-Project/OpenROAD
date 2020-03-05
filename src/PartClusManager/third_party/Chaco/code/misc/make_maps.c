/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>


int       make_maps(setlists, list_ptrs, set, glob2loc, loc2glob)
int      *setlists;		/* linked list of set vertices */
int      *list_ptrs;		/* head of each linked list */
int       set;			/* set value denoting subgraph */
int      *glob2loc;		/* graph -> subgraph numbering map */
int      *loc2glob;		/* subgraph -> graph numbering map */
{
    int       i, j;		/* loop counter */

    j = 0;
    i = list_ptrs[set];

    if (glob2loc != NULL) {
	while (i != 0) {
	    loc2glob[++j] = i;
	    glob2loc[i] = j;
	    i = setlists[i];
	}
    }

    else {
	while (i != 0) {
	    loc2glob[++j] = i;
	    i = setlists[i];
	}
    }

    return (j);
}



void      make_maps2(assignment, nvtxs, set, glob2loc, loc2glob)
short    *assignment;		/* set assignments for graph */
int       nvtxs;		/* length of assignment */
int       set;			/* set value denoting subgraph */
int      *glob2loc;		/* graph -> subgraph numbering map */
int      *loc2glob;		/* subgraph -> graph numbering map */
{
    int       i, j;		/* loop counter */

    j = 0;
    if (glob2loc != NULL) {
	for (i = 1; i <= nvtxs; i++) {
	    if (assignment[i] == set) {
		j++;
		glob2loc[i] = j;
		loc2glob[j] = i;
	    }
	}
    }
    else {
	for (i = 1; i <= nvtxs; i++) {
	    if (assignment[i] == set) {
		j++;
		loc2glob[j] = i;
	    }
	}
    }
}
