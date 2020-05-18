/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"structs.h"
#include	"defs.h"


void      count_weights(graph, nvtxs, sets, nsets, weights, using_vwgts)
struct vtx_data **graph;	/* data structure for graph */
int       nvtxs;		/* number of vtxs in graph */
short    *sets;			/* set each vertex is assigned to */
int       nsets;		/* number of sets in this division */
double   *weights;		/* vertex weights in each set */
int       using_vwgts;		/* are vertex weights being used? */

{
    int       i;		/* loop counters */

    /* Compute the sum of vertex weights for each set. */
    for (i = 0; i < nsets; i++)
	weights[i] = 0;

    if (using_vwgts) {
        for (i = 1; i <= nvtxs; i++) {
	    weights[sets[i]] += graph[i]->vwgt;
	}
    }

    else {
        for (i = 1; i <= nvtxs; i++) {
	    weights[sets[i]]++;
	}
    }
}
