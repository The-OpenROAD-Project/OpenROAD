/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"defs.h"
#include	"structs.h"


static int bfsearch();

int       find_edges(graph, nvtxs, mark, vtxlist, edges)
/* Breadth first seach algorithm to find connected components. */
struct vtx_data **graph;	/* graph data structure */
int       nvtxs;		/* number of vertices in graph */
short    *mark;			/* space for nvtxs+1 ints */
int      *vtxlist;		/* space for nvtxs ints */
struct edgeslist **edges;	/* list of edges connecting graph */
{
    struct edgeslist *newedge;	/* space to add new edge */
    int       root;		/* vertex to start the dfs */
    int       last;		/* last vertex seen in BFS */
    int       count;		/* number of vertices seen so far */
    int       nadded;		/* number of edges needed to be added */
    int       i;		/* loop counter */
    double   *smalloc();
    double    drandom();

    for (i = 1; i <= nvtxs; i++)
	mark[i] = -1;
    count = 0;
    nadded = 0;
    *edges = NULL;
    root = nvtxs * drandom() + 1;

    last = bfsearch(graph, root, &count, mark, vtxlist, nadded);

    while (count != nvtxs) {	/* Are there any remaining vertices? */
	/* Find starting vtx for next BFS. */
	root = nvtxs * drandom() + 1;
	while (mark[root] >= 0) {
	    root++;
	    if (root > nvtxs)
		root = 1;
	}
	/* Add new edge to list needed for connectivity. */
	newedge = (struct edgeslist *) smalloc(sizeof(struct edgeslist));
	newedge->next = *edges;
	newedge->vtx1 = last;
	newedge->vtx2 = root;
	*edges = newedge;
	nadded++;
	last = bfsearch(graph, root, &count, mark, vtxlist, nadded);
    }
    return (nadded);
}


/* BFS to find connected component */
static int bfsearch(graph, root, count, mark, vtxlist, comp_num)
struct vtx_data **graph;	/* graph data structure */
int       root;			/* start vertex for DFS */
int      *count;		/* number of vertices in component */
short    *mark;			/* has vtx been seen? */
int      *vtxlist;		/* space for storing vtxs to search */
int       comp_num;		/* current component number */
{
    int      *iptr;		/* loops through neighbor list */
    int       vtxbeg, vtxend;	/* beginning and end of vertices in vtxlist */
    int       vtx;		/* vertex being processed */
    int       neighbor;		/* neighbor of vertex */
    int       i;		/* loop counter */

    vtxbeg = vtxend = 1;
    mark[root] = (short) comp_num;
    vtxlist[0] = root;

    /* Copy root's neighbors to vtxlist, incrementing count */
    iptr = graph[root]->edges;
    for (i = graph[root]->nedges - 1; i; i--) {
	neighbor = *(++iptr);
	vtxlist[vtxend++] = neighbor;
	mark[neighbor] = (short) comp_num;
    }

    while (vtxbeg < vtxend) {
	vtx = vtxlist[vtxbeg++];
	/* Loop through neighbors, copying to vtxlist if unmarked. */
	iptr = graph[vtx]->edges;
	for (i = graph[vtx]->nedges - 1; i; i--) {
	    neighbor = *(++iptr);
	    if (mark[neighbor] != comp_num) {
		mark[neighbor] = (short) comp_num;
		vtxlist[vtxend++] = neighbor;
	    }
	}
    }
    *count += vtxend;
    return (vtxlist[vtxend - 1]);
}
