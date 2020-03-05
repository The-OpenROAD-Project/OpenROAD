/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include <math.h>
#include "defs.h"
#include "structs.h"


/* Check graph for errors */


int       check_graph(graph, nvtxs, nedges)
struct vtx_data **graph;	/* graph data structure */
int       nvtxs;		/* number of vertices */
int       nedges;		/* number of edges */
{
    extern FILE *Output_File;   /* output file or null */
    float     eweight;		/* edge weight */
    double    wgt_sum;		/* sum of edge weights */
    int       flag;		/* flag for error free graph */
    int       no_edge_count;	/* warning flag for isolated vertex */
    int       bad_vwgt_count;	/* number of vertices with bad vwgts */
    int       using_ewgts;	/* are edge weights being used? */
    int       narcs;		/* number of neighbors of a vertex */
    int       neighbor;		/* neighbor of a vertex */
    int       i, j;		/* loop counters */
    int       is_an_edge();

    flag = FALSE;
    no_edge_count = 0;
    bad_vwgt_count = 0;
    using_ewgts = (graph[1]->ewgts != NULL);
    narcs = 0;
    for (i = 1; i <= nvtxs; i++) {
	narcs += graph[i]->nedges - 1;

	if (graph[i]->edges[0] != i) {
	    printf(" Self edge wrong for vtx %d\n", i);
	    flag = TRUE;
	}

	if (graph[i]->nedges == 1) {
	    if (!no_edge_count) {
		printf("WARNING: Vertex %d has no neighbors\n", i);
    		if (Output_File != NULL) {
		    fprintf(Output_File,
			"WARNING: Vertex %d has no neighbors\n", i);
		}
	    }
	    ++no_edge_count;
	}

	if (graph[i]->vwgt <= 0) {
	    if (!bad_vwgt_count)
		printf("Vertex %d has bad vertex weight %d.\n",
		       i, graph[i]->vwgt);
	    ++bad_vwgt_count;
	    flag = TRUE;
	}

	if (using_ewgts)
	    wgt_sum = graph[i]->ewgts[0];

	for (j = 1; j < graph[i]->nedges; j++) {
	    neighbor = graph[i]->edges[j];
	    if (using_ewgts)
		wgt_sum += graph[i]->ewgts[j];

/* Move it to the end and delete instead? */
	    if (neighbor == i) {
		printf("Self edge (%d,%d) not allowed\n", i, neighbor);
		flag = TRUE;
	    }

	    if (neighbor < 1 || neighbor > nvtxs) {
		printf("Edge (%d,%d) included, but nvtxs = %d\n",
		       i, neighbor, nvtxs);
		flag = TRUE;
	    }

/* Move it to the end and delete instead? */
	    if (using_ewgts && graph[i]->ewgts[j] <= 0) {
		printf("Bad edge weight %g for edge (%d, %d)\n",
		       graph[i]->ewgts[j], i, neighbor);
		flag = TRUE;
	    }

	    if (!is_an_edge(graph[neighbor], i, &eweight)) {
		printf("Edge (%d,%d) included but not (%d,%d)\n",
		       i, neighbor, neighbor, i);
		flag = TRUE;
	    }
	    else if (using_ewgts && eweight != graph[i]->ewgts[j]) {
		printf("Weight of (%d,%d)=%g, but weight of (%d,%d)=%g\n",
		       i, neighbor, graph[i]->ewgts[j], neighbor, i, eweight);
		flag = TRUE;
	    }
	}

	if (using_ewgts && fabs(wgt_sum) > 1.0e-7 * fabs(graph[i]->ewgts[0])) {
	    printf("Sum of edge weights for vertex %d = %g\n", i, wgt_sum);
	    flag = TRUE;
	}
    }

    if (no_edge_count > 1) {
	printf("WARNING: %d vertices have no neighbors\n", no_edge_count);
        if (Output_File != NULL) {
	    fprintf(Output_File,
		"WARNING: %d vertices have no neighbors\n", no_edge_count);
	}
    }

    if (bad_vwgt_count > 1)
	printf("%d vertices have bad vertex weights\n", bad_vwgt_count);

    if (narcs != 2 * nedges) {
	printf(" twice nedges = %d, but I count %d\n", 2 * nedges, narcs);
	flag = TRUE;
    }
    return (flag);
}


int       is_an_edge(vertex, v2, weight2)
struct vtx_data *vertex;	/* data for a vertex */
int       v2;			/* neighbor to look for */
float    *weight2;		/* weight of edge if found */
{
    int       i;		/* loop counter */

    for (i = 1; i < vertex->nedges; i++) {
	if (vertex->edges[i] == v2) {
	    if (vertex->ewgts != NULL)
		*weight2 = vertex->ewgts[i];
	    else
		*weight2 = 1;
	    return (TRUE);
	}
    }

    return (FALSE);
}
