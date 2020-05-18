/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include "defs.h"
#include "structs.h"


/* Find a maximal matching in a graph using Luby's algorithm.  Assign a */
/* random value to each edge and include an edge in the matching if it  */
/* has a higher value than all neighboring edges that aren't disallowed. */
/* (Use float instead of double values to save space.) */
/* THIS ROUTINE IS SLOWER THAN MAXMATCH3, AND SO PROBABLY OBSOLETE. */

int       maxmatch4(graph, nvtxs, nedges, mflag, using_ewgts)
/* Find a maximal matching in the graph, using variant of Luby's algorithm. */
/* Assign a random value to each edge, and include an edge in */
/* the matching if its value larger than all neighboring edges. */
struct vtx_data **graph;	/* array of vtx data for graph */
int       nvtxs;		/* number of vertices in graph */
int       nedges;		/* number of edges in graph */
int      *mflag;		/* flag indicating vtx selected or not */
{
    extern int HEAVY_MATCH;	/* try for heavy edges in matching? */
    int      *iptr;		/* loops through integer arrays */
    float    *edgevals;		/* random values for all edges */
    float    *evptr;		/* loops through edgevals */
    double    maxval;		/* largest edge value for a vertex */
    int       neighbor;		/* neighbor of a vertex */
    int       nmerged;		/* number of edges in matching */
    int       change;		/* any new edges in matching? */
    int      *start;		/* start of edgevals list for each vertex */
    int       i, j, k;		/* loop counters */
    double   *smalloc();
    int       sfree();
    double    drandom();

    /* Allocate and initialize space. */
    evptr = edgevals = (float *) smalloc((unsigned) 2 * nedges * sizeof(float));

    start = (int *) smalloc((unsigned) (nvtxs + 2) * sizeof(int));
    start[1] = 0;
    for (i = 1; i <= nvtxs; i++)
	start[i + 1] = start[i] + graph[i]->nedges - 1;

    /* Assign a random value to each edge. */
    if (!using_ewgts || !HEAVY_MATCH) {	/* All edges are equal. */
	for (i = 1; i <= nvtxs; i++) {
	    for (j = 1; j < graph[i]->nedges; j++) {
		neighbor = graph[i]->edges[j];
		if (neighbor > i) {
		    *evptr = (float) drandom();
		}
		else {		/* Look up already-generated value. */
		    for (k = 1; graph[neighbor]->edges[k] != i; k++);
		    *evptr = edgevals[start[neighbor] + k - 1];
		}
		evptr++;
	    }
	}
    }
    else {			/* Prefer heavy weight edges. */
	for (i = 1; i <= nvtxs; i++) {
	    for (j = 1; j < graph[i]->nedges; j++) {
		neighbor = graph[i]->edges[j];
		if (neighbor > i) {
		    *evptr = (float) graph[i]->ewgts[j] * drandom();
		}
		else {		/* Look up already-generated value. */
		    for (k = 1; graph[neighbor]->edges[k] != i; k++);
		    *evptr = edgevals[start[neighbor] + k - 1];
		}
		evptr++;
	    }
	}
    }

    for (iptr = mflag, i = nvtxs; i; i--)
	*(++iptr) = -(nvtxs + 1);
    nmerged = 0;
    change = TRUE;
    while (change) {
	change = FALSE;

	for (i = 1; i <= nvtxs; i++) {	/* Find largest valued edge of each vtx */
	    if (mflag[i] < 0) {
		maxval = 0.0;
		k = -1;
		evptr = &(edgevals[start[i]]);
		for (j = 1; j < graph[i]->nedges; j++) {
		    if (*evptr > maxval && mflag[graph[i]->edges[j]] < 0) {
			maxval = *evptr;
			k = j;
		    }
		    evptr++;
		}
		if (k == -1)
		    mflag[i] = 0;	/* No neighbors are alive. */
		else {
		    mflag[i] = -graph[i]->edges[k];
		}
	    }
	}

	/* If vtxs agree on largest valued edge, add to independent set. */
	for (i = 1; i <= nvtxs; i++) {
	    if (-mflag[i] > i) {
		if (-mflag[-mflag[i]] == i) {	/* Add edge to independent set. */
		    nmerged++;
		    mflag[i] = -mflag[i];
		    mflag[mflag[i]] = i;
		    change = TRUE;
		}
	    }
	}
    }

    /* Maximal independent set is indicated by corresponding pairs */
    /* of positive values in the mflag array. */
    for (i = 1; i <= nvtxs; i++)
	if (mflag[i] < 0)
	    mflag[i] = 0;

    sfree((char *) start);
    sfree((char *) edgevals);
    return (nmerged);
}
