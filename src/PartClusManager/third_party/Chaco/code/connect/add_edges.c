/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"defs.h"
#include	"structs.h"


void      add_edges(graph, new_edges, old_edges, old_ewgts, using_ewgts)
struct vtx_data **graph;	/* graph data structure */
struct edgeslist *new_edges;	/* list of edges connecting graph */
struct ilists **old_edges;	/* edges data overwritten for connecting */
struct flists **old_ewgts;	/* weights of edges overwritten */
int       using_ewgts;		/* are edge weights being used? */
{
    struct ilists *save_list;	/* space to save old edge list */
    struct flists *save_ewgts;	/* space to save old edge weights */
    struct edgeslist *edges;	/* loops through new edges */
    float    *new_ewgts;	/* new edge weights */
    int      *new_list;		/* new edge list */
    int       nedges;		/* number of edges a vertex has */
    int       vtx, vtx2;	/* two vertices in edge to be added */
    int       i, j;		/* loop counter */
    double   *smalloc();

    *old_edges = NULL;
    *old_ewgts = NULL;
    edges = new_edges;
    while (edges != NULL) {
	for (j = 0; j < 2; j++) {
	    if (j == 0) {
		vtx = edges->vtx1;
		vtx2 = edges->vtx2;
	    }
	    else {
		vtx = edges->vtx2;
		vtx2 = edges->vtx1;
	    }

	    /* Copy old edge list to new edge list. */
	    nedges = graph[vtx]->nedges;
	    new_list = (int *) smalloc((unsigned) (nedges + 1) * sizeof(int));
	    for (i = 0; i < nedges; i++)
		new_list[i] = graph[vtx]->edges[i];
	    new_list[nedges] = vtx2;

	    /* Save old edges. */
	    save_list = (struct ilists *) smalloc((unsigned) sizeof(struct ilists));
	    save_list->list = graph[vtx]->edges;

	    /* Add new list at FRONT of linked list to facilitate uncoarsening. */
	    save_list->next = *old_edges;
	    *old_edges = save_list;

	    /* Now modify graph to have new edges list. */
	    graph[vtx]->nedges++;
	    graph[vtx]->edges = new_list;

	    /* If using edge weights, I have to modify those too. */
	    if (using_ewgts) {
		new_ewgts = (float *) smalloc((unsigned) (nedges + 1) * sizeof(float));
		for (i = 1; i < nedges; i++)
		    new_ewgts[i] = graph[vtx]->ewgts[i];
		new_ewgts[nedges] = 1;
		new_ewgts[0] = graph[vtx]->ewgts[0] - new_ewgts[nedges];

		/* Save old edge weights. */
		save_ewgts = (struct flists *) smalloc((unsigned) sizeof(struct flists));
		save_ewgts->list = graph[vtx]->ewgts;

		save_ewgts->next = *old_ewgts;
		*old_ewgts = save_ewgts;

		/* Finally, modify graph to have new edge weights. */
		graph[vtx]->ewgts = new_ewgts;
	    }
	}
	edges = edges->next;
    }
}
