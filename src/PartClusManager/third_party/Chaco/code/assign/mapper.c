/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"params.h"
#include	"defs.h"
#include	"structs.h"


void      mapper(graph, xvecs, nvtxs, active, sets, ndims, cube_or_mesh, nsets,
		           mediantype, goal, vwgt_max)
struct vtx_data **graph;	/* data structure with vertex weights */
double  **xvecs;		/* continuous indicator vectors */
int       nvtxs;		/* number of vtxs in graph */
int      *active;		/* space for nmyvals ints */
short    *sets;			/* returned processor assignment for my vtxs */
int       ndims;		/* number of dimensions being divided into */
int       cube_or_mesh;		/* 0 => hypercube, d => d-dimensional mesh */
int       nsets;		/* number of sets to divide into */
int       mediantype;		/* type of eigenvector partitioning to use */
double   *goal;			/* desired set sizes */
int       vwgt_max;		/* largest vertex weight */
{
    double    temp_goal[2];	/* combined set goals if using option 1. */
    double    wbelow;		/* weight of vertices with negative values */
    double    wabove;		/* weight of vertices with positive values */
    short    *temp_sets;	/* sets vertices get assigned to */
    int       vweight;		/* weight of a vertex */
    int       using_vwgts;	/* are vertex weights being used? */
    int       bits;		/* bits for assigning set numbers */
    int       i, j;		/* loop counters */
    int       sfree();
    double   *smalloc();
    void      median(), rec_median_k(), rec_median_1(), median_assign();
    void      map2d(), map3d();

    /* NOTE: THIS EXPECTS XVECS, NOT YVECS! */

    using_vwgts = (vwgt_max != 1);

    if (ndims == 1 && mediantype == 1)
	mediantype = 3;		/* simpler call than normal option 1. */

    if (mediantype == 0) {	/* Divide at zero instead of median. */
	bits = 1;
	temp_sets = (short *) smalloc((unsigned) (nvtxs + 1) * sizeof(short));
	for (j = 1; j <= nvtxs; j++)
	    sets[j] = 0;

	for (i = 1; i <= ndims; i++) {
	    temp_goal[0] = temp_goal[1] = 0;
	    for (j = 0; j < (1 << ndims); j++) {
		if (bits & j)
		    temp_goal[1] += goal[j];
		else
		    temp_goal[0] += goal[j];
	    }
	    bits <<= 1;

	    wbelow = wabove = 0;
	    vweight = 1;
	    for (j = 1; j <= nvtxs; j++) {
		if (using_vwgts)
		    vweight = graph[j]->vwgt;
		if (xvecs[i][j] < 0)
		    wbelow += vweight;
		else if (xvecs[i][j] > 0)
		    wabove += vweight;
	    }

	    median_assign(graph, xvecs[i], nvtxs, temp_goal, using_vwgts, temp_sets,
			  wbelow, wabove, (double) 0.0);

	    for (j = 1; j <= nvtxs; j++)
		sets[j] = (sets[j] << 1) + temp_sets[j];
	}
    }

    else if (mediantype == 1) {	/* Divide using min-cost assignment. */
	if (ndims == 2)
	    map2d(graph, xvecs, nvtxs, sets, goal, vwgt_max);
	else if (ndims == 3)
	    map3d(graph, xvecs, nvtxs, sets, goal, vwgt_max);
    }


    else if (mediantype == 2) {	/* Divide recursively using medians. */
	rec_median_k(graph, xvecs, nvtxs, active, ndims, cube_or_mesh, goal,
		     using_vwgts, sets);
    }

    else if (mediantype == 3) {	/* Cut with independent medians => unbalanced. */
	bits = 1;
	temp_sets = (short *) smalloc((unsigned) (nvtxs + 1) * sizeof(short));
	for (j = 1; j <= nvtxs; j++)
	    sets[j] = 0;

	for (i = 1; i <= ndims; i++) {
	    temp_goal[0] = temp_goal[1] = 0;
	    for (j = 0; j < (1 << ndims); j++) {
		if (bits & j)
		    temp_goal[1] += goal[j];
		else
		    temp_goal[0] += goal[j];
	    }
	    bits <<= 1;

	    median(graph, xvecs[i], nvtxs, active, temp_goal, using_vwgts, temp_sets);
	    for (j = 1; j <= nvtxs; j++)
		sets[j] = (sets[j] << 1) + temp_sets[j];
	}
	sfree((char *) temp_sets);
    }

    if (mediantype == 4) {	/* Stripe the domain. */
	rec_median_1(graph, xvecs[1], nvtxs, active, cube_or_mesh, nsets,
		     goal, using_vwgts, sets, TRUE);
    }
}
