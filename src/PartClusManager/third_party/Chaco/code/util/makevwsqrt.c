/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include <math.h>
#include "structs.h"


void      makevwsqrt(vwsqrt, graph, nvtxs)
/* Make vector of square roots of vertex weights. */
double   *vwsqrt;		/* vector returned */
struct vtx_data **graph;	/* graph data structure */
int       nvtxs;		/* number of vertices in graph */
{
    extern int NSQRTS;		/* number of sqrts already computed */
    extern double *SQRTS;	/* values computed */
    int       vwgt;		/* vertex weight */
    int       i;		/* loop counter */

    for (i = 1; i <= nvtxs; i++) {
	vwgt = graph[i]->vwgt;
	if (vwgt <= NSQRTS)
	    vwsqrt[i] = SQRTS[vwgt];
	else
	    vwsqrt[i] = sqrt((double) vwgt);
    }
}


/* Extract the subgraph vwsqrt values */
void      make_subvector(vec, subvec, subnvtxs, loc2glob)
double   *vec;			/* vector for all vertices */
double   *subvec;		/* vector for vertices in subgraph */
int       subnvtxs;		/* number of vtxs in subgraph */
int      *loc2glob;		/* subgraph -> graph numbering map */
{
    int       i;

    for (i = 1; i <= subnvtxs; i++) {
	++subvec;
	(*subvec) = vec[loc2glob[i]];
    }
}
