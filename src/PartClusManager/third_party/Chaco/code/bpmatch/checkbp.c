/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	<math.h>
#include	"structs.h"
#include	"params.h"
#include	"defs.h"


/* Confirm that the bipartite match algorithm did the right thing. */
void      checkbp(graph, xvecs, sets, dists, nvtxs, ndims)
struct vtx_data **graph;	/* graph data structure for vertex weights */
double  **xvecs;		/* values to partition */
short    *sets;			/* set assignments returned */
double   *dists;		/* distances that separate sets */
int       nvtxs;		/* number of vertices */
int       ndims;		/* number of dimensions for division */
{
    int       signs[MAXDIMS];	/* signs for coordinates of target points */
    int       sizes[MAXSETS];	/* size of each set */
    int       weights[MAXSETS];	/* size of each set */
    double    setval;		/* value from assigned set */
    double    val, bestval;	/* value to decide set assignment */
    double    tol = 1.0e-8;	/* numerical tolerence */
    int       error = FALSE;	/* are errors encountered? */
    int       nsets;		/* number of sets */
    int       bestset;		/* set vtx should be assigned to */
    int       i, j, k;		/* loop counter */
    void      checkpnt();

    nsets = 1 << ndims;

    for (i = 0; i < nsets; i++) {
	sizes[i] = 0;
	weights[i] = 0;
    }

    for (i = 1; i <= nvtxs; i++) {
	/* Is vertex closest to the set it is assigned to? */
	for (j = 0; j < ndims; j++)
	    signs[j] = -1;
	bestval = 0;
	for (j = 0; j < nsets; j++) {
	    val = -dists[j];
	    for (k = 1; k <= ndims; k++) {
		val += 2 * signs[k - 1] * xvecs[k][i];
	    }
	    if (j == sets[i])
		setval = val;
	    if (j == 0 || val < bestval) {
		bestval = val;
		bestset = j;
	    }
	    if (signs[0] == 1 && signs[1] == 1)
		signs[2] *= -1;
	    if (signs[0] == 1)
		signs[1] *= -1;
	    signs[0] *= -1;
	}
	if (fabs(setval - bestval) >= tol * (fabs(setval) + fabs(bestval))) {
	    printf(" Vtx %d in set %d (%e), but should be in %d (%e)\n",
		   i, sets[i], setval, bestset, bestval);
	    error = TRUE;
	}
	++sizes[sets[i]];
	weights[sets[i]] += graph[i]->vwgt;
    }

    printf(" Sizes:");
    for (i = 0; i < nsets; i++)
	printf(" %d(%d)", sizes[i], weights[i]);
    printf("\n");

    if (error)
	checkpnt("ERROR in checkbp");
}
