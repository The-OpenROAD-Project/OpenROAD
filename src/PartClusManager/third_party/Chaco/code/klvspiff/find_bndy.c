/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include "defs.h"
#include "structs.h"


/* Find vertices on boundary of partition, and change their assignments. */

int       find_bndy(graph, nvtxs, assignment, new_val, pbndy_list)
struct vtx_data **graph;	/* array of vtx data for graph */
int       nvtxs;		/* number of vertices in graph */
short    *assignment;		/* processor each vertex gets assigned to */
int       new_val;		/* assignment value for boundary vtxs */
int     **pbndy_list;		/* returned list, end with zero */
{
    int      *bndy_list;	/* returned list, end with zero */
    int      *edges;		/* loops through edge list */
    int       list_length;	/* returned number of vtxs on boundary */
    int       set, set2;	/* set a vertex is in */
    int       i, j;		/* loop counters */
    double   *smalloc(), *srealloc();

    bndy_list = (int *) smalloc((unsigned) (nvtxs + 1) * sizeof(int));

    list_length = 0;
    for (i = 1; i <= nvtxs; i++) {
	set = assignment[i];
	edges = graph[i]->edges;
	for (j = graph[i]->nedges - 1; j; j--) {
	    set2 = assignment[*(++edges)];
	    if (set2 != set) {
		bndy_list[list_length++] = i;
		break;
	    }
	}
    }
    bndy_list[list_length] = 0;

    for (i = 0; i < list_length; i++) {
	assignment[bndy_list[i]] = (short) new_val;
    }

    /* Shrink out unnecessary space */
    *pbndy_list = (int *) srealloc((char *) bndy_list,
				 (unsigned) (list_length + 1) * sizeof(int));

    return (list_length);
}


/* Find a vertex separator on one side of an edge separator. */

int       find_side_bndy(graph, nvtxs, assignment, side, new_val, pbndy_list)
struct vtx_data **graph;	/* array of vtx data for graph */
int       nvtxs;		/* number of vertices in graph */
short    *assignment;		/* processor each vertex gets assigned to */
int       side;			/* side to take vertices from */
int       new_val;		/* assignment value for boundary vtxs */
int     **pbndy_list;		/* returned list, end with zero */

{
    int      *edges;		/* loops through edge list */
    int      *bndy_list;	/* returned list, end with zero */
    int       list_length;	/* returned number of vtxs on boundary */
    int       set, set2;	/* set a vertex is in */
    int       i, j;		/* loop counters */
    double   *smalloc(), *srealloc();

    if (*pbndy_list != NULL) {
	/* Contains list of all vertices on boundary. */
	bndy_list = *pbndy_list;
	i = list_length = 0;
	while (bndy_list[i] != 0) {
	    if (assignment[bndy_list[i]] == side) {
		bndy_list[list_length++] = bndy_list[i];
	    }
	    ++i;
	}
    }

    else {
	bndy_list = (int *) smalloc((unsigned) (nvtxs + 1) * sizeof(int));

	list_length = 0;
	for (i = 1; i <= nvtxs; i++) {
	    set = assignment[i];
	    if (set == side) {
		edges = graph[i]->edges;
		for (j = graph[i]->nedges - 1; j; j--) {
		    set2 = assignment[*(++edges)];
		    if (set2 != set) {
			bndy_list[list_length++] = i;
			break;
		    }
		}
	    }
	}
    }

    bndy_list[list_length] = 0;

    for (i = 0; i < list_length; i++) {
	assignment[bndy_list[i]] = (short) new_val;
    }

    /* Shrink out unnecessary space */
    *pbndy_list = (int *) srealloc((char *) bndy_list,
				 (unsigned) (list_length + 1) * sizeof(int));

    return (list_length);
}
