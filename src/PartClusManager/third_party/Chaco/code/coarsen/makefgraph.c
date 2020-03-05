/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include "defs.h"
#include "structs.h"

static void makecv2v();

void      makefgraph(graph, nvtxs, nedges, pcgraph, cnvtxs, pcnedges, v2cv,
		               using_ewgts, igeom, coords, ccoords)
struct vtx_data **graph;	/* array of vtx data for graph */
int       nvtxs;		/* number of vertices in graph */
int       nedges;		/* number of edges in graph */
struct vtx_data ***pcgraph;	/* coarsened version of graph */
int       cnvtxs;		/* number of vtxs in coarsened graph */
int      *pcnedges;		/* number of edges in coarsened graph */
int      *v2cv;			/* mapping from vtxs to coarsened vtxs */
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
    int      *cv2v_vals;	/* vtxs corresponding to each cvtx */
    int      *cv2v_ptrs;	/* indices into cv2v_vals */
    float    *eweights;		/* space for edge weights in coarsened graph */
    float    *ewptr;		/* loops through eweights */
    float    *fptr;		/* loops through eweights */
    float     ewgt;		/* edge weight */
    double    ewgt_sum;		/* sum of edge weights */
    double    time;		/* timing parameters */
    int       nseen;		/* number of edges of coarse graph seen so far */
    int       vtx;		/* vertex in original graph */
    int       cvtx;		/* vertex in coarse graph */
    int       cnedges;		/* twice number of edges in coarsened graph */
    int       neighbor;		/* neighboring vertex */
    int       size;		/* space needed for coarsened graph */
    int      *edges;		/* space for edges in coarsened graph */
    int      *eptr;		/* loops through edges data structure */
    int       cneighbor;	/* neighboring vertex number in coarsened graph */
    int       i, j;		/* loop counters */
    double   *smalloc(), *srealloc();
    double    seconds();
    int       sfree();
    void      makeccoords();

    /* Compute the number of vertices and edges in the coarsened graph, */
    /* and construct start pointers into coarsened edge array. */
    time = seconds();

    /* Construct mapping from original graph vtxs to coarsened graph vtxs. */
    cv2v_vals = (int *) smalloc((unsigned) nvtxs * sizeof(int));
    cv2v_ptrs = (int *) smalloc((unsigned) (cnvtxs + 2) * sizeof(int));
    makecv2v(nvtxs, cnvtxs, v2cv, cv2v_vals, cv2v_ptrs);

    /* Compute an upper bound on the number of coarse graph edges. */
    cnedges = nedges - (nvtxs - cnvtxs);

    /* Now allocate space for the new graph.  Overallocate and realloc later. */
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
    cnedges = 0;
    eptr = edges;
    ewgt = 1;

    sptr = cv2v_vals;
    for (cvtx = 1; cvtx <= cnvtxs; cvtx++) {
	nseen = 1;

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

	ewgt_sum = 0;
	for (i = cv2v_ptrs[cvtx + 1] - cv2v_ptrs[cvtx]; i; i--) {
	    vtx = *sptr++;

	    iptr = graph[vtx]->edges;
	    if (using_ewgts)
		fptr = graph[vtx]->ewgts;
	    for (j = graph[vtx]->nedges - 1; j; j--) {
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
	}

	/* Now clear the seenflag values. */
	iptr = cgptr->edges;
	for (j = nseen - 1; j; j--) {
	    seenflag[*(++iptr)] = 0;
	}

	if (COARSEN_EWGTS)
	    cgptr->ewgts[0] = -ewgt_sum;
	/* Increment pointers into edges list. */
	cgptr->nedges = nseen;
	eptr += nseen;
	if (COARSEN_EWGTS) {
	    ewptr += nseen;
	}

	cnedges += nseen - 1;
    }

    sfree((char *) seenflag);

    /* Form new vertex weights by adding those from contracted edges. */
    if (COARSEN_VWGTS) {
	gptr = graph;
	for (i = 1; i <= nvtxs; i++) {
	    cgraph[v2cv[i]]->vwgt += (*(++gptr))->vwgt;
	}
    }

    /* Reduce arrays to actual sizes */
    cnedges /= 2;
    size = 2 * cnedges + cnvtxs;
    eptr = edges;
    edges = (int *) srealloc((char *) edges, (unsigned) size * sizeof(int));
    if (eptr != edges) {        /* Need to reset pointers in graph. */
        for (i = 1; i <= cnvtxs; i++) {
            cgraph[i]->edges = edges;
            edges += cgraph[i]->nedges;
        }
    }

    if (COARSEN_EWGTS) {
	ewptr = eweights;
	eweights = (float *) srealloc((char *) eweights,
				      (unsigned) size * sizeof(float));
        if (ewptr != eweights) {        /* Need to reset pointers in graph. */
            for (i = 1; i <= cnvtxs; i++) {
                cgraph[i]->ewgts = eweights;
                eweights += cgraph[i]->nedges;
            }
        }
    }

    /* If desired, make new vtx coordinates = center-of-mass of their parents. */
    if (coords != NULL && ccoords != NULL && igeom > 0) {
	makeccoords(graph, cnvtxs, cv2v_ptrs, cv2v_vals,
		    igeom, coords, ccoords);
    }

    *pcnedges = cnedges;

    sfree((char *) cv2v_ptrs);
    sfree((char *) cv2v_vals);

    if (DEBUG_COARSEN > 0) {
	printf(" Coarse graph has %d vertices and %d edges\n", cnvtxs, cnedges);
    }

    make_cgraph_time += seconds() - time;
}



static void makecv2v(nvtxs, cnvtxs, v2cv, cv2v_vals, cv2v_ptrs)
int       nvtxs;		/* number of vertices in graph */
int       cnvtxs;		/* number of vtxs in coarsened graph */
int      *v2cv;			/* mapping from vtxs to coarsened vtxs */
int      *cv2v_vals;		/* vtxs corresponding to each cvtx */
int      *cv2v_ptrs;		/* indices into cv2c_vals */

{
    int       sum;		/* cumulative offests into vals array */
    int       i;		/* loop counter */

    /* First find number of vtxs associated with each coarse graph vtx. */

    for (i = 1; i <= cnvtxs + 1; i++) {
	cv2v_ptrs[i] = 0;
    }

    for (i = 1; i <= nvtxs; i++) {
	++cv2v_ptrs[v2cv[i] + 1];	/* +1 offsets and simplifies next loop. */
    }
    cv2v_ptrs[1] = 0;

    /* Now make this a cumulative total to index into cv2v_vals. */
    sum = 0;
    for (i = 2; i <= cnvtxs + 1; i++) {
	cv2v_ptrs[i] += sum;
	sum = cv2v_ptrs[i];
    }

    /* Now go ahead and set the cv2v_vals. */
    for (i = 1; i <= nvtxs; i++) {
	cv2v_vals[cv2v_ptrs[v2cv[i]]] = i;
	++cv2v_ptrs[v2cv[i]];
    }

    /* Finally, reset the cv2v_ptrs values. */
    for (i = cnvtxs; i; i--) {
	cv2v_ptrs[i] = cv2v_ptrs[i - 1];
    }
    cv2v_ptrs[1] = 0;
}
