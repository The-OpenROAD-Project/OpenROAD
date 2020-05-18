/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include "defs.h"
#include "structs.h"


void      countcedges(graph, nvtxs, start, seenflag, mflag, v2cv, pcnedges)
/* Count edges in coarsened graph and construct start array. */
struct vtx_data **graph;	/* array of vtx data for graph */
int       nvtxs;		/* number of vertices in graph */
int      *start;		/* start of edgevals list for each vertex */
int      *seenflag;		/* flags indicating vtxs already counted */
int      *mflag;		/* flag indicating vtx matched or not */
int      *v2cv;			/* mapping from fine to coarse vertices */
int      *pcnedges;		/* number of edges in coarsened graph */
{
    int      *jptr;		/* loops through edge list */
    int       cnedges;		/* twice number of edges in coarsened graph */
    int       neighbor;		/* neighboring vertex */
    int       cneighbor;	/* neighboring vertex in coarse graph */
    int       nneighbors;	/* number of neighboring vertices */
    int       newi;		/* loops over vtxs in coarsened graph */
    int       i, j;		/* loop counters */

    /* Note: seenflag values already set to zero. */

    cnedges = 0;
    newi = 1;
    start[1] = 0;
    for (i = 1; i <= nvtxs; i++) {
	if (mflag[i] == 0 || mflag[i] > i) {
	    nneighbors = 0;
	    jptr = graph[i]->edges;
	    for (j = graph[i]->nedges - 1; j; j--) {
		/* Has edge already been added? */
		neighbor = *(++jptr);
		if (neighbor != mflag[i]) {
		    cneighbor = v2cv[neighbor];

		    if (seenflag[cneighbor] != i) {
			nneighbors++;
			seenflag[cneighbor] = i;
		    }
		}
	    }

	    if (mflag[i] > i) {	/* Take care of matching vertex. */
		jptr = graph[mflag[i]]->edges;
		for (j = graph[mflag[i]]->nedges - 1; j; j--) {
		    neighbor = *(++jptr);
		    if (neighbor != i) {
			cneighbor = v2cv[neighbor];

			if (seenflag[cneighbor] != i) {
			    nneighbors++;
			    seenflag[cneighbor] = i;
			}
		    }
		}
	    }

	    start[newi + 1] = start[newi] + nneighbors;
	    newi++;
	    cnedges += nneighbors;
	}
    }
    *pcnedges = cnedges / 2;
}
