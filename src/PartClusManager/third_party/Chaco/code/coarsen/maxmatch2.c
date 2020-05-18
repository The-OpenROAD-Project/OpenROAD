/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include "defs.h"
#include "structs.h"


/* Find a maximal matching in a graph using simple greedy algorithm. */
/* Randomly permute vertices, and then have each select first unmatched */
/* neighbor. */

int       maxmatch2(graph, nvtxs, mflag, using_ewgts)
struct vtx_data **graph;	/* array of vtx data for graph */
int       nvtxs;		/* number of vertices in graph */
int      *mflag;		/* flag indicating vtx selected or not */
int       using_ewgts;		/* are edge weights being used? */
{
    extern int HEAVY_MATCH;	/* encourage heavy matching edges? */
    float     ewgt_max;		/* heaviest edge seen so far */
    int      *order;		/* random ordering of vertices */
    int      *iptr, *jptr;	/* loops through integer arrays */
    int       matched;		/* has vertex been matched? */
    int       vtx;		/* vertex to process next */
    int       neighbor;		/* neighbor of a vertex */
    int       nmerged;		/* number of edges in matching */
    int       jsave;		/* best edge so far */
    int       i, j;		/* loop counters */
    int       sfree();
    double   *smalloc();
    void      randomize();

    /* First, randomly permute the vertices. */
    iptr = order = (int *) smalloc((unsigned) (nvtxs + 1) * sizeof(int));
    jptr = mflag;
    for (i = 1; i <= nvtxs; i++) {
	*(++iptr) = i;
	*(++jptr) = 0;
    }
    randomize(order, nvtxs);

    nmerged = 0;

    if (!using_ewgts || !HEAVY_MATCH) {	/* Simple greedy approach. */
	for (i = 1; i <= nvtxs; i++) {
	    vtx = order[i];
	    if (mflag[vtx] == 0) {	/* Not already matched. */
		/* Find first unmatched neighbor of vtx. */
		matched = FALSE;
		for (j = 1; j < graph[vtx]->nedges && !matched; j++) {
		    neighbor = graph[vtx]->edges[j];
		    if (mflag[neighbor] == 0) {	/* Found one! */
			mflag[vtx] = neighbor;
			mflag[neighbor] = vtx;
			matched = TRUE;
			nmerged++;
		    }
		}
	    }
	}
    }

    else {			/* Find heavy edge to match */
	for (i = 1; i <= nvtxs; i++) {
	    vtx = order[i];
	    if (mflag[vtx] == 0) {	/* Not already matched. */
		/* Find heaviest unmatched neighbor of vtx. */
		jsave = 0;
		ewgt_max = 0;
		for (j = 1; j < graph[vtx]->nedges; j++) {
		    neighbor = graph[vtx]->edges[j];
		    if (mflag[neighbor] == 0 && graph[vtx]->ewgts[j] > ewgt_max) {
			ewgt_max = graph[vtx]->ewgts[j];
			jsave = j;
		    }
		}
		if (jsave > 0) {
		    neighbor = graph[vtx]->edges[jsave];
		    mflag[vtx] = neighbor;
		    mflag[neighbor] = vtx;
		    nmerged++;
		}
	    }
	}
    }

    sfree((char *) order);
    return (nmerged);
}
