/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include "defs.h"
#include "structs.h"


void      makecgraph(graph, nvtxs, pcgraph, pcnvtxs, pcnedges, mflag,
		                v2cv, nmerged, using_ewgts, igeom, coords, ccoords)
struct vtx_data **graph;	/* array of vtx data for graph */
int       nvtxs;		/* number of vertices in graph */
struct vtx_data ***pcgraph;	/* coarsened version of graph */
int      *pcnvtxs;		/* number of vtxs in coarsened graph */
int      *pcnedges;		/* number of edges in coarsened graph */
int      *mflag;		/* flag indicating vtx matched or not */
int      *v2cv;			/* mapping from vtxs to coarsened vtxs */
int       nmerged;		/* number of merged vertices */
int       using_ewgts;		/* are edge weights being used? */
int       igeom;		/* dimensions of geometric data */
float   **coords;		/* coordinates for vertices */
float   **ccoords;		/* coordinates for coarsened vertices */
{
    extern double make_cgraph_time;
    extern int DEBUG_COARSEN;	/* debug flag for coarsening output */
    extern int COARSEN_VWGTS;	/* turn off vertex weights in coarse graph? */
    extern int COARSEN_EWGTS;	/* turn off edge weights in coarse graph? */
    struct vtx_data **cgraph;	/* coarsened version of graph */
    struct vtx_data *links;	/* space for all the vertex data */
    struct vtx_data **gptr;	/* loops through cgraph */
    struct vtx_data *cgptr;	/* loops through cgraph */
    int      *start;		/* start of edgevals list for each vertex */
    int      *iptr;		/* loops through integer arrays */
    int      *seenflag;		/* flags for vtxs already put in edge list */
    int      *sptr;		/* loops through seenflags */
    float    *eweights;		/* space for edge weights in coarsened graph */
    float    *fptr;		/* loops through eweights */
    float     ewgt;		/* edge weight */
    double    ewgt_sum;		/* sum of edge weights */
    double    time;		/* timing parameters */
    int       nseen;		/* number of edges of coarse graph seen so far */
    int       cnvtxs;		/* number of vtxs in coarsened graph */
    int       cnedges;		/* twice number of edges in coarsened graph */
    int       neighbor;		/* neighboring vertex */
    int       size;		/* space needed for coarsened graph */
    int      *edges;		/* space for edges in coarsened graph */
    int       cvtx;		/* vertex number in coarsened graph */
    int       cneighbor;	/* neighboring vertex number in coarsened graph */
    int       i, j;		/* loop counters */
    double   *smalloc();
    double    seconds();
    int       sfree();
    void      makev2cv(), countcedges();

    /* Compute the number of vertices and edges in the coarsened graph, */
    /* and construct start pointers into coarsened edge array. */
    time = seconds();

    *pcnvtxs = cnvtxs = nvtxs - nmerged;

    /* Construct mapping from original graph vtxs to coarsened graph vtxs. */
    makev2cv(mflag, nvtxs, v2cv);

    start = (int *) smalloc((unsigned) (cnvtxs + 2) * sizeof(int));

    seenflag = (int *) smalloc((unsigned) (cnvtxs + 1) * sizeof(int));
    sptr = seenflag;
    for (i = cnvtxs; i; i--)
	*(++sptr) = 0;

    countcedges(graph, nvtxs, start, seenflag, mflag, v2cv, pcnedges);
    cnedges = *pcnedges;

    if (DEBUG_COARSEN > 0) {
	printf(" Coarse graph has %d vertices and %d edges\n", cnvtxs, cnedges);
    }

    /* Now allocate space for the new graph. */
    *pcgraph = cgraph = (struct vtx_data **) smalloc((unsigned) (cnvtxs + 1) * sizeof(struct vtx_data *));
    links = (struct vtx_data *) smalloc((unsigned) cnvtxs * sizeof(struct vtx_data));

    size = 2 * cnedges + cnvtxs;
    edges = (int *) smalloc((unsigned) size * sizeof(int));
    if (COARSEN_EWGTS) {
	eweights = (float *) smalloc((unsigned) size * sizeof(float));
    }

    /* Fill in simple data fields for coarsened graph. */
    /* Edges and weights are put in later. */
    gptr = cgraph;
    sptr = seenflag;
    for (i = 1; i <= cnvtxs; i++) {
	size = start[i + 1] - start[i] + 1;
	links->nedges = size;
	links->edges = edges;
	links->edges[0] = i;
	edges += size;
	if (COARSEN_VWGTS)
	    links->vwgt = 0;
	else
	    links->vwgt = 1;
	if (COARSEN_EWGTS) {
	    links->ewgts = eweights;
	    eweights += size;
	}
	else
	    links->ewgts = NULL;
	*(++gptr) = links++;
	*(++sptr) = 0;
    }
    sfree((char *) start);

    /* Now form new vertex weights by adding those from contracted edges. */
    if (COARSEN_VWGTS) {
	gptr = graph;
	for (i = 1; i <= nvtxs; i++) {
	    cgraph[v2cv[i]]->vwgt += (*(++gptr))->vwgt;
	}
    }

    /* Use the renumbering to fill in the edge lists for the new graph. */
    ewgt = 1;
    for (i = 1; i <= nvtxs; i++) {
	if (mflag[i] > i || mflag[i] == 0) {
	    /* Unmatched edge, or first appearance of matched edge. */
	    nseen = 1;
	    cvtx = v2cv[i];
	    cgptr = cgraph[cvtx];
	    ewgt_sum = 0;

	    iptr = graph[i]->edges;
	    if (using_ewgts)
		fptr = graph[i]->ewgts;
	    for (j = graph[i]->nedges - 1; j; j--) {
		neighbor = *(++iptr);
		cneighbor = v2cv[neighbor];
		if (cneighbor != cvtx) {
		    if (using_ewgts)
			ewgt = *(++fptr);
		    ewgt_sum += ewgt;

		    /* Seenflags being used as map from cvtx to index. */
		    if (seenflag[cneighbor] == 0) {	/* New neighbor. */
			cgptr->edges[nseen] = cneighbor;
			if (COARSEN_EWGTS)
			    cgptr->ewgts[nseen] = ewgt;
			seenflag[cneighbor] = nseen++;
		    }
		    else {	/* Already seen neighbor. */
			if (COARSEN_EWGTS)
			    cgptr->ewgts[seenflag[cneighbor]] += ewgt;
		    }
		}
		else if (using_ewgts)
		    ++fptr;
	    }

	    if (mflag[i] > i) {	/* Now handle the matched vertex. */
		iptr = graph[mflag[i]]->edges;
		if (using_ewgts)
		    fptr = graph[mflag[i]]->ewgts;
		for (j = graph[mflag[i]]->nedges - 1; j; j--) {
		    neighbor = *(++iptr);
		    cneighbor = v2cv[neighbor];
		    if (cneighbor != cvtx) {
			if (using_ewgts)
			    ewgt = *(++fptr);
			ewgt_sum += ewgt;

			if (seenflag[cneighbor] == 0) {	/* New neighbor. */
			    cgptr->edges[nseen] = cneighbor;
			    if (COARSEN_EWGTS)
				cgptr->ewgts[nseen] = ewgt;
			    seenflag[cneighbor] = nseen++;
			}
			else {	/* Already seen neighbor. */
			    if (COARSEN_EWGTS)
				cgptr->ewgts[seenflag[cneighbor]] += ewgt;
			}
		    }
		    else if (using_ewgts)
			++fptr;
		}
	    }
	    if (COARSEN_EWGTS)
		cgptr->ewgts[0] = -ewgt_sum;
	    /* Now clear the seenflag values. */
	    iptr = cgraph[cvtx]->edges;
	    for (j = cgraph[cvtx]->nedges - 1; j; j--) {
		seenflag[*(++iptr)] = 0;
	    }
	}
    }

    sfree((char *) seenflag);

    /* If desired, make new vtx coordinates = center-of-mass of their parents. */
    if (coords != NULL && ccoords != NULL && igeom > 0) {
	makeccoords(graph, nvtxs, cnvtxs, mflag, v2cv, igeom, coords, ccoords);
    }

    make_cgraph_time += seconds() - time;
}
