/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include "defs.h"
#include "structs.h"

/* Find the maximum weighted degree of a vertex. */
double    find_maxdeg(graph, nvtxs, using_ewgts, pmax_ewgt)
struct vtx_data **graph;	/* graph data structure */
int       nvtxs;		/* number of vertices */
int       using_ewgts;		/* are edge weights being used? */
float    *pmax_ewgt;		/* returns largest edge weight if not NULL */
{
    double    maxdeg;		/* maximum degree of a vertex */
    float    *eptr;		/* loops through edge weights */
    float     max_ewgt;		/* largest edge weight in graph */
    float     ewgt;		/* edge weight value */
    int       i, j;		/* loop counter */

    /* Find the maximum weighted degree of a vertex. */
    maxdeg = 0;
    if (using_ewgts) {
	if (pmax_ewgt != NULL) {
	    max_ewgt = 0;
	    for (i = 1; i <= nvtxs; i++) {
		if (-graph[i]->ewgts[0] > maxdeg)
		    maxdeg = -graph[i]->ewgts[0];

		eptr = graph[i]->ewgts;
		for (j = graph[i]->nedges - 1; j; j--) {
		    ewgt = *(++eptr);
		    if (ewgt > max_ewgt)
			max_ewgt = ewgt;
		}
	    }
	    *pmax_ewgt = max_ewgt;
	}
	else {
	    for (i = 1; i <= nvtxs; i++) {
		if (-graph[i]->ewgts[0] > maxdeg)
		    maxdeg = -graph[i]->ewgts[0];
	    }
	}
    }

    else {
	for (i = 1; i <= nvtxs; i++) {
	    if (graph[i]->nedges > maxdeg)
		maxdeg = graph[i]->nedges - 1;
	}
	if (pmax_ewgt != NULL)
	    *pmax_ewgt = 1;
    }
    return (maxdeg);
}
