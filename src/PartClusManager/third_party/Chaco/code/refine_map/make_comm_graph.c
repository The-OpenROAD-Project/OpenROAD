/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	<string.h>
#include	"structs.h"
#include	"defs.h"


/* Construct a weighted quotient graph representing the inter-set communication. */
int       make_comm_graph(pcomm_graph, graph, nvtxs, using_ewgts, assign, nsets_tot)
struct vtx_data ***pcomm_graph;	/* graph for communication requirements */
struct vtx_data **graph;	/* graph data structure */
int       nvtxs;		/* number of vertices in graph */
int       using_ewgts;		/* are edge weights being used? */
short    *assign;		/* current assignment */
int       nsets_tot;		/* total number of sets */
{
    float     ewgt;		/* edge weight in graph */
    int     **edges_list = NULL;	/* lists of edges */
    int     **ewgts_list = NULL;	/* lists of edge weights */
    int      *edges;		/* edges in communication graph */
    int      *ewgts;		/* edge weights in communication graph */
    float    *float_ewgts = NULL;	/* edge weights in floating point */
    int      *adj_sets = NULL;	/* weights connecting sets */
    int      *order = NULL;	/* ordering of vertices by set */
    int      *sizes = NULL;	/* sizes of different sets */
    int      *start = NULL;	/* pointers into adjacency data */
    int      *adjacency = NULL;	/* array with all the edge info */
    int      *eptr;		/* loops through edges in graph */
    int      *ewptr;		/* loop through edge weights */
    int       set, set2;	/* sets two vertices belong to */
    int       vertex;		/* vertex in graph */
    int       ncomm_edges;	/* number of edges in communication graph */
    int       error;		/* out of space? */
    int       i, j;		/* loop counters */
    double   *smalloc_ret();
    int       sfree(), reformat();

    error = 1;
    *pcomm_graph = NULL;

    /* First construct some mappings to ease later manipulations. */
    sizes = (int *) smalloc_ret((unsigned) nsets_tot * sizeof(int));
    if (sizes == NULL) goto skip;

    for (i = 0; i < nsets_tot; i++)
	sizes[i] = 0;
    for (i = 1; i <= nvtxs; i++)
	++(sizes[assign[i]]);

    /* Now make sizes reflect the start index for each set. */
    for (i = 1; i < nsets_tot - 1; i++)
	sizes[i] += sizes[i - 1];
    for (i = nsets_tot - 1; i; i--)
	sizes[i] = sizes[i - 1];
    sizes[0] = 0;

    /* Now construct list of all vertices in set 0, all in set 1, etc. */
    order = (int *) smalloc_ret((unsigned) nvtxs * sizeof(int));
    if (order == NULL) goto skip;
    for (i = 1; i <= nvtxs; i++) {
	set = assign[i];
	order[sizes[set]] = i;
	++sizes[set];
    }

    /* For each set, find total weight to all neighbors. */
    adj_sets = (int *) smalloc_ret((unsigned) nsets_tot * sizeof(int));
    edges_list = (int **) smalloc_ret((unsigned) nsets_tot * sizeof(int *));
    ewgts_list = (int **) smalloc_ret((unsigned) nsets_tot * sizeof(int *));
    start = (int *) smalloc_ret((unsigned) (nsets_tot + 1) * sizeof(int));
    if (adj_sets == NULL || edges_list == NULL || ewgts_list == NULL ||
	start == NULL) goto skip;

    start[0] = 0;
    ewgt = 1;
    ncomm_edges = 0;

    for (set = 0; set < nsets_tot; set++) {
	edges_list[set] = NULL;
	ewgts_list[set] = NULL;
    }

    for (set = 0; set < nsets_tot; set++) {
	for (i = 0; i < nsets_tot; i++)
	    adj_sets[i] = 0;
	for (i = (set ? sizes[set - 1] : 0); i < sizes[set]; i++) {
	    vertex = order[i];
	    for (j = 1; j < graph[vertex]->nedges; j++) {
		set2 = assign[graph[vertex]->edges[j]];
		if (set2 != set) {
		    if (using_ewgts)
			ewgt = graph[vertex]->ewgts[j];
		    adj_sets[set2] += ewgt;

		}
	    }
	}

	/* Now save adj_sets data to later construct graph. */
	j = 0;
	for (i = 0; i < nsets_tot; i++)
	    if (adj_sets[i])
		j++;
	ncomm_edges += j;
	start[set + 1] = ncomm_edges;
	if (j) {
	    edges_list[set] = edges = (int *) smalloc_ret((unsigned) j * sizeof(int));
	    ewgts_list[set] = ewgts = (int *) smalloc_ret((unsigned) j * sizeof(int));
	    if (edges == NULL || ewgts == NULL) goto skip;
	}
	j = 0;
	for (i = 0; i < nsets_tot; i++) {
	    if (adj_sets[i]) {
		edges[j] = i + 1;
		ewgts[j] = adj_sets[i];
		j++;
	    }
	}
    }

    sfree((char *) adj_sets);
    sfree((char *) order);
    sfree((char *) sizes);
    adj_sets = order = sizes = NULL;

    /* I now need to pack the edge and weight data into single arrays. */
    adjacency = (int *) smalloc_ret((unsigned) (ncomm_edges + 1) * sizeof(int));
    float_ewgts = (float *) smalloc_ret((unsigned) (ncomm_edges + 1) * sizeof(float));
    if (adjacency == NULL || float_ewgts == NULL) goto skip;

    for (set = 0; set < nsets_tot; set++) {
	j = start[set];
	eptr = edges_list[set];
	ewptr = ewgts_list[set];
	for (i = start[set]; i < start[set + 1]; i++) {
	    adjacency[i] = eptr[i - j];
	    float_ewgts[i] = ewptr[i - j];
	}
	if (start[set] != start[set + 1]) {
	    sfree((char *) edges_list[set]);
	    sfree((char *) ewgts_list[set]);
	}
    }
    sfree((char *) edges_list);
    sfree((char *) ewgts_list);
    edges_list = ewgts_list = NULL;

    error = reformat(start, adjacency, nsets_tot, &ncomm_edges, (int *) NULL,
	     float_ewgts, pcomm_graph);

skip:
    sfree((char *) adj_sets);
    sfree((char *) order);
    sfree((char *) sizes);
    if (edges_list != NULL) {
        for (set = nsets_tot-1; set >= 0; set--) {
	    if (edges_list[set] != NULL) sfree((char *) edges_list[set]);
	}
        sfree((char *) edges_list);
    }

    if (ewgts_list != NULL) {
        for (set = nsets_tot-1; set >= 0; set--) {
	    if (ewgts_list[set] != NULL) sfree((char *) ewgts_list[set]);
	}
        sfree((char *) ewgts_list);
    }

    sfree((char *) float_ewgts);
    sfree((char *) adjacency);
    sfree((char *) start);

    return(error);
}
