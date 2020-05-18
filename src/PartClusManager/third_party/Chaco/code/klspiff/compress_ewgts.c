/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"structs.h"
#include	"params.h"
#include	"defs.h"


static float *old_ewgts;	/* space for old edge weights */


void      compress_ewgts(graph, nvtxs, nedges, ewgt_max, using_ewgts)
struct vtx_data **graph;	/* list of graph info for each vertex */
int       nvtxs;		/* number of vertices in graph */
int       nedges;		/* number of edges in graph */
float     ewgt_max;		/* largest edge weight */
int       using_ewgts;		/* are edge weights being used? */
{
    extern double EWGT_RATIO_MAX;	/* max allowed ewgt/nvtxs */
    float    *old_ewptr;	/* loops through old edge weights */
    float    *new_ewptr;	/* loops through old edge weights */
    float    *self_ptr;		/* points to self edge in new_ewgts */
    float    *new_ewgts;	/* space for new edge weights */
    int       ewgt;		/* new edge weight value */
    double    sum;		/* sum of all the new edge weights */
    double    ratio;		/* amount to reduce edge weights */
    int       i, j;		/* loop counter */
    double   *smalloc();

    /* Check easy cases first. */
    if (!using_ewgts)
	old_ewgts = NULL;

    else if (ewgt_max < EWGT_RATIO_MAX * nvtxs) {
	/* If not too heavy, leave it alone. */
	old_ewgts = NULL;
	printf("In compress_ewgts, but not too heavy, ewgt_max = %g, nvtxs = %d\n", ewgt_max, nvtxs);
    }

    else {			/* Otherwise, compress edge weights. */
	/* Allocate space for new edge weights */
	old_ewgts = graph[1]->ewgts;
	new_ewgts = (float *) smalloc((unsigned) (2 * nedges + nvtxs) * sizeof(float));
	ratio = (EWGT_RATIO_MAX * nvtxs) / ewgt_max;
	printf("In compress_ewgts, ewgt_max = %g, nvtxs = %d, ratio = %e\n", ewgt_max, nvtxs, ratio);
	old_ewptr = old_ewgts;
	new_ewptr = new_ewgts;
	for (i = 1; i <= nvtxs; i++) {
	    self_ptr = new_ewptr++;
	    old_ewptr++;
	    sum = 0;
	    for (j = graph[i]->nedges - 1; j; j--) {
		ewgt = (int) (*(old_ewptr++) * ratio + 1.0);
		*(new_ewptr++) = (float) ewgt;
		sum += (float) ewgt;
	    }
	    *self_ptr = -sum;
	    graph[i]->ewgts = self_ptr;
	}
    }
}


void      restore_ewgts(graph, nvtxs)
struct vtx_data **graph;	/* list of graph info for each vertex */
int       nvtxs;		/* number of vertices in graph */
{
    int       i;		/* loop counter */
    int       sfree();

    /* Check easy case first. */
    if (old_ewgts == NULL)
	return;

    else {			/* otherwise, compress edge weights. */
	sfree((char *) graph[1]->ewgts);
	for (i = 1; i <= nvtxs; i++) {
	    graph[i]->ewgts = old_ewgts;
	    old_ewgts += graph[i]->nedges;
	}

	old_ewgts = NULL;
    }
}
