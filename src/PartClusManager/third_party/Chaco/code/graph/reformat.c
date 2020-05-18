/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include "defs.h"
#include "structs.h"


/* Change from a FORTRAN graph style to our graph data structure. */

int       reformat(start, adjacency, nvtxs, pnedges, vwgts, ewgts, pgraph)
int      *start;		/* start of edge list for each vertex */
int      *adjacency;		/* edge list data */
int       nvtxs;		/* number of vertices in graph */
int      *pnedges;		/* ptr to number of edges in graph */
int      *vwgts;		/* weights for all vertices */
float    *ewgts;		/* weights for all edges */
struct vtx_data ***pgraph;	/* ptr to array of vtx data for graph */
{
    extern FILE *Output_File;           /* output file or null */
    struct vtx_data **graph = NULL;	/* array of vtx data for graph */
    struct vtx_data *links = NULL;	/* space for data for all vtxs */
    int      *edges = NULL;	/* space for all adjacency lists */
    float    *eweights = NULL;	/* space for all edge weights */
    int      *eptr;		/* steps through adjacency list */
    int      *eptr_save;	/* saved index into adjacency list */
    float    *wptr;		/* steps through edge weights list */
    int       self_edge;	/* number of self loops detected */
    int       size;		/* length of all edge lists */
    double    sum;		/* sum of edge weights for a vtx */
    int       using_ewgts;	/* are edge weights being used? */
    int       using_vwgts;	/* are vertex weights being used? */
    int       i, j;		/* loop counters */
    double   *smalloc_ret();

    using_ewgts = (ewgts != NULL);
    using_vwgts = (vwgts != NULL);

    graph = (struct vtx_data **) smalloc_ret((unsigned) (nvtxs + 1) * sizeof(struct vtx_data *));
    *pgraph = graph;
    if (graph == NULL) return(1);

    graph[1] = NULL;

    /* Set up all the basic data structure for the vertices. */
    /* Replace many small mallocs by a few large ones. */
    links = (struct vtx_data *) smalloc_ret((unsigned) (nvtxs) * sizeof(struct vtx_data));
    if (links == NULL) return(1);

    for (i = 1; i <= nvtxs; i++) {
	graph[i] = links++;
    }

    graph[1]->edges = NULL;
    graph[1]->ewgts = NULL;

    /* Now fill in all the data fields. */
    if (start != NULL)
	*pnedges = start[nvtxs] / 2;
    else
	*pnedges = 0;
    size = 2 * (*pnedges) + nvtxs;
    edges = (int *) smalloc_ret((unsigned) size * sizeof(int));
    if (edges == NULL) return(1);

    if (using_ewgts) {
	eweights = (float *) smalloc_ret((unsigned) size * sizeof(float));
        if (eweights == NULL) return(1);
    }

    if (start != NULL) {
        eptr = adjacency + start[0];
        wptr = ewgts;
    }
    self_edge = 0;

    for (i = 1; i <= nvtxs; i++) {
	if (using_vwgts)
	    graph[i]->vwgt = *(vwgts++);
	else
	    graph[i]->vwgt = 1;
	if (start != NULL)
	    size = start[i] - start[i - 1];
	else
	    size = 0;
	graph[i]->nedges = size + 1;
	graph[i]->edges = edges;
	*edges++ = i;
	eptr_save = eptr;
	for (j = size; j; j--) {
	    if (*eptr != i)
		*edges++ = *eptr++;
	    else {		/* Self edge, skip it. */
		if (!self_edge) {
		    printf("WARNING: Self edge (%d,%d) being ignored\n", i, i);
    		    if (Output_File != NULL) {
		        fprintf(Output_File,
			    "WARNING: Self edge (%d,%d) being ignored\n", i, i);
		    }
		}
		++self_edge;
		eptr++;
		--(graph[i]->nedges);
		--(*pnedges);
	    }
	}
	if (using_ewgts) {
	    graph[i]->ewgts = eweights;
	    eweights++;
	    sum = 0;
	    for (j = size; j; j--) {
		if (*eptr_save++ != i) {
		    sum += *wptr;
		    *eweights++ = *wptr++;
		}
		else
		    wptr++;
	    }
	    graph[i]->ewgts[0] = -sum;
	}
	else
	    graph[i]->ewgts = NULL;
    }
    if (self_edge > 1) {
	printf("WARNING: %d self edges were detected and ignored\n", self_edge);
        if (Output_File != NULL) {
	    fprintf(Output_File,
		"WARNING: %d self edges were detected and ignored\n", self_edge);
	}
    }

    return(0);
}
