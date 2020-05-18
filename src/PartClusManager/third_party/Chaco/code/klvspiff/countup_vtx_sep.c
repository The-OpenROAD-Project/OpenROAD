/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"structs.h"
#include	"params.h"
#include	"defs.h"


void      countup_vtx_sep(graph, nvtxs, sets)
struct vtx_data **graph;	/* list of graph info for each vertex */
int       nvtxs;		/* number of vertices in graph */
short    *sets;			/* local partitioning of vtxs */
{
    int       vtx, set;		/* vertex and set in graph */
    int       sep_size;		/* size of the separator */
    int       i, j, k;		/* loop counters */

    sep_size = 0;
    j = k = 0;
    for (i = 1; i <= nvtxs; i++) {
	if (sets[i] == 0)
	    j += graph[i]->vwgt;
	if (sets[i] == 1)
	    k += graph[i]->vwgt;
	if (sets[i] == 2)
	    sep_size += graph[i]->vwgt;
    }
    printf("Set sizes = %d/%d, Separator size = %d\n\n", j, k, sep_size);

    /* Now check that it really is a separator. */
    for (i = 1; i <= nvtxs; i++) {
	set = sets[i];
	if (set != 2) {
	    for (j = 1; j < graph[i]->nedges; j++) {
		vtx = graph[i]->edges[j];
		if (sets[vtx] != 2 && sets[vtx] != set) {
		    printf("Error: %d (set %d) adjacent to %d (set %d)\n",
			   i, set, vtx, sets[vtx]);
		}
	    }
	}
    }
}
