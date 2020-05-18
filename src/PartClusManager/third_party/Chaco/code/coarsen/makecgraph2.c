/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include "defs.h"
#include "structs.h"


void      makecgraph2(graph, nvtxs, nedges, pcgraph, pcnvtxs, pcnedges, mflag,
		               v2cv, nmerged, using_ewgts, igeom, coords, ccoords)
struct vtx_data **graph;	/* array of vtx data for graph */
int       nvtxs;		/* number of vertices in graph */
int       nedges;		/* number of edges in graph */
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
    int      *iptr;		/* loops through integer arrays */
    int      *seenflag;		/* flags for vtxs already put in edge list */
    int      *sptr;		/* loops through seenflags */
    float    *eweights;		/* space for edge weights in coarsened graph */
    float    *ewptr;		/* loops through eweights */
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
    int      *eptr;		/* loops through edges data structure */
    int       cvtx;		/* vertex number in coarsened graph */
    int       cneighbor;	/* neighboring vertex number in coarsened graph */
    double    m1, m2;		/* vertex weights of vertices being merged */
    int       v1, v2;		/* vertices being merged */
    int       i, j;		/* loop counters */
    double   *smalloc(), *srealloc();
    double    seconds();
    int       sfree();
    void      makev2cv();

    /* Compute the number of vertices and edges in the coarsened graph, */
    /* and construct start pointers into coarsened edge array. */
    time = seconds();

    *pcnvtxs = cnvtxs = nvtxs - nmerged;

    /* Construct mapping from original graph vtxs to coarsened graph vtxs. */
    makev2cv(mflag, nvtxs, v2cv);

    /* Compute an upper bound on the number of coarse graph edges. */
    cnedges = nedges - nmerged;

    /* Now allocate space for the new graph. */
    *pcgraph = cgraph = (struct vtx_data **) smalloc((unsigned) (cnvtxs + 1) * sizeof(struct vtx_data *));
    links = (struct vtx_data *) smalloc((unsigned) cnvtxs * sizeof(struct vtx_data));

    size = 2 * cnedges + cnvtxs;
    edges = (int *) smalloc((unsigned) size * sizeof(int));
    if (COARSEN_EWGTS) {
	ewptr = eweights = (float *) smalloc((unsigned) size * sizeof(float));
    }

    /* Zero all the seen flags. */
    seenflag = (int *) smalloc((unsigned) (cnvtxs + 1) * sizeof(int));
    sptr = seenflag;
    for (i = cnvtxs; i; i--) {
	*(++sptr) = 0;
    }

    /* Use the renumbering to fill in the edge lists for the new graph. */
    /* Note: This assumes coarse graph vertices are encountered in order. */
    cnedges = 0;
    eptr = edges;
    ewgt = 1;
    sptr = seenflag;

    for (i = 1; i <= nvtxs; i++) {
	if (mflag[i] > i || mflag[i] == 0) {
	    /* Unmatched edge, or first appearance of matched edge. */
	    nseen = 1;
	    cvtx = v2cv[i];
	    cgptr = cgraph[cvtx] = links++;

	    if (COARSEN_VWGTS)
		cgptr->vwgt = 0;
	    else
		cgptr->vwgt = 1;

	    eptr[0] = cvtx;
	    cgptr->edges = eptr;
	    if (COARSEN_EWGTS) {
		cgptr->ewgts = ewptr;
	    }
	    else
		cgptr->ewgts = NULL;
	    *(++sptr) = 0;

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
	    /* Increment pointers into edges list. */
	    cgptr->nedges = nseen;
	    eptr += nseen;
	    if (COARSEN_EWGTS) {
		ewptr += nseen;
	    }

	    /* Now clear the seenflag values. */
	    iptr = cgptr->edges;
	    for (j = nseen - 1; j; j--) {
		seenflag[*(++iptr)] = 0;
	    }

	    cnedges += nseen - 1;
	}
    }

    sfree((char *) seenflag);

    /* Form new vertex weights by adding those from contracted edges. */
    if (COARSEN_VWGTS) {
	gptr = graph;
	for (i = 1; i <= nvtxs; i++) {
	    cgraph[v2cv[i]]->vwgt += (*(++gptr))->vwgt;
	}
    }

    /* If desired, make new vtx coordinates = center-of-mass of their parents. */
    if (coords != NULL && ccoords != NULL && igeom > 0) {
	for (i = 0; i < igeom; i++) {
	    ccoords[i] = (float *) smalloc((unsigned) (cnvtxs + 1) * sizeof(float));
	}
	for (j = 1; j <= nvtxs; j++) {
	    if (mflag[j] == 0) {/* If unmatched, leave it alone. */
		for (i = 0; i < igeom; i++) {
		    ccoords[i][v2cv[j]] = coords[i][j];
		}
	    }
	    else if (j < mflag[j]) {	/* If matched, use center of mass. */
		v1 = j;
		v2 = mflag[j];
		m1 = graph[v1]->vwgt;
		m2 = graph[v2]->vwgt;
		for (i = 0; i < igeom; i++) {
		    ccoords[i][v2cv[j]] = (m1 * coords[i][v1] + m2 * coords[i][v2]) / (m1 + m2);
		}
	    }
	}
    }

    cnedges /= 2;
    *pcnedges = cnedges;
    size = 2 * cnedges + cnvtxs;
    edges = (int *) srealloc((char *) edges, (unsigned) size * sizeof(int));
    if (COARSEN_EWGTS) {
	eweights = (float *) srealloc((char *) eweights,
	    (unsigned) size * sizeof(float));
    }

    if (DEBUG_COARSEN > 0) {
	printf(" Coarse graph has %d vertices and %d edges\n", cnvtxs, cnedges);
    }

    make_cgraph_time += seconds() - time;
}
