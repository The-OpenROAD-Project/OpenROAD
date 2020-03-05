/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	<math.h>
#include	"structs.h"
#include	"defs.h"


void      clear_dvals(graph, nvtxs, ldvals, rdvals, bspace, list_length)
struct vtx_data **graph;	/* data structure for graph */
int       nvtxs;		/* number of vtxs in graph */
int      *ldvals;		/* d-values for each transition */
int      *rdvals;		/* d-values for each transition */
int      *bspace;		/* list of activated vertices */
int       list_length;		/* number of activated vertices */
{
    int      *edges;		/* loops through edge list */
    int       vtx;		/* vertex in bspace */
    int       neighbor;		/* neighbor of vtx */
    int       i, j;		/* loop counters */

    if (list_length > .05 * nvtxs) {	/* Do it directly. */
	for (i = 1; i <= nvtxs; i++) {
	    ldvals[i] = rdvals[i] = 0;
	}
    }
    else {			/* Do it more carefully */
	for (i = 0; i < list_length; i++) {
	    vtx = bspace[i];
	    if (vtx < 0)
		vtx = -vtx;
	    ldvals[vtx] = rdvals[vtx] = 0;
	    edges = graph[vtx]->edges;
	    for (j = graph[vtx]->nedges - 1; j; j--) {
		neighbor = *(++edges);
		ldvals[neighbor] = rdvals[neighbor] = 0;
	    }
	}
    }
}
