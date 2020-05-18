/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"structs.h"
#include	"params.h"
#include	"defs.h"


/* Construct a graph representing the inter-set communication. */
int       refine_part(graph, nvtxs, using_ewgts, assign, architecture,
    ndims_tot, mesh_dims, goal)
struct vtx_data **graph;	/* graph data structure */
int       nvtxs;		/* number of vertices in graph */
int       using_ewgts;		/* are edge weights being used? */
short    *assign;		/* current assignment */
int       architecture;		/* 0 => hypercube, d => d-dimensional mesh */
int       ndims_tot;		/* if hypercube, number of dimensions */
int       mesh_dims[3];		/* if mesh, size in each direction */
double   *goal;			/* desired set sizes */
{
    extern int TERM_PROP;	/* perform terminal propagation? */
    struct bilist *set_list = NULL;	/* lists of vtxs in each set */
    struct bilist *vtx_elems = NULL;	/* space for all vtxs in set_lists */
    struct bilist *ptr;		/* loops through set_lists */
    struct ipairs *pairs = NULL;/* ordered list of edges in comm graph */
    double   *comm_vals = NULL;	/* edge wgts of comm graph for sorting */
    float    *term_wgts[2];	/* terminal propagation vector */
    short     hops[MAXSETS][MAXSETS];	/* preference weighting */
    double   *temp;		/* return argument from srealloc_ret() */
    int      *indices = NULL;	/* sorted order for communication edges */
    int      *space = NULL;	/* space for mergesort */
    int      *sizes = NULL;	/* sizes of the different sets */
    short    *sub_assign = NULL;	/* new assignment for subgraph */
    short    *old_sub_assign = NULL;	/* room for current sub assignment */
    int     **edges_list = NULL;/* lists of comm graph edges */
    int     **ewgts_list = NULL;/* lists of comm graph edge wgts */
    int      *ewgts;		/* loops through ewgts_list */
    int      *edges;		/* edges in communication graph */
    int      *adj_sets = NULL;	/* weights connecting sets */
    int      *eptr;		/* loop through edges and edge weights */
    int      *ewptr;		/* loop through edges and edge weights */
    int       ewgt;		/* weight of an edge */
    struct vtx_data **subgraph = NULL;	/* subgraph data structure */
    int      *nedges = NULL;	/* space for saving graph data */
    short    *degrees = NULL;	/* # neighbors of vertices */
    int      *glob2loc = NULL;	/* maps full to reduced numbering */
    int      *loc2glob = NULL;	/* maps reduced to full numbering */
    int       nmax;		/* largest subgraph I expect to encounter */
    int       set, set1, set2;	/* sets vertices belong to */
    int       vertex;		/* vertex in graph */
    int       ncomm;		/* # edges in communication graph */
    int       dist;		/* architectural distance between two sets */
    int       nsets_tot;	/* total number of processors */
    int       change;		/* did change occur in this pass? */
    int       any_change;	/* has any change occured? */
    int       error;		/* out of space? */
    int       size;		/* array spacing */
    int       i, j, k;		/* loop counters */
    double   *smalloc_ret(), *srealloc_ret();
    int       sfree(), abs(), kl_refine();
    void      mergesort(), strout();

    error = 1;
    term_wgts[1] = NULL;

    if (architecture == 0) {
	nsets_tot = 1 << ndims_tot;
    }
    else if (architecture > 0) {
	nsets_tot = mesh_dims[0] * mesh_dims[1] * mesh_dims[2];
    }

    hops[0][0] = hops[1][1] = 0;
    if (!TERM_PROP) {
        hops[0][1] = hops[1][0] = 1;
    }

    /* Set up convenient data structure for navigating through sets. */
    set_list = (struct bilist *) smalloc_ret((unsigned) nsets_tot * sizeof(struct bilist));
    vtx_elems = (struct bilist *)
	smalloc_ret((unsigned) (nvtxs + 1) * sizeof(struct bilist));
    sizes = (int *) smalloc_ret((unsigned) nsets_tot * sizeof(int));
    if (set_list == NULL || vtx_elems == NULL || sizes == NULL) goto skip;


    for (i = 0; i < nsets_tot; i++) {
	set_list[i].next = NULL;
	sizes[i] = 0;
    }

    for (i = 1; i <= nvtxs; i++) {
	set = assign[i];
	++sizes[set];
	vtx_elems[i].next = set_list[set].next;
	if (vtx_elems[i].next != NULL) {
	    vtx_elems[i].next->prev = &(vtx_elems[i]);
	}
	vtx_elems[i].prev = &(set_list[set]);
	set_list[set].next = &(vtx_elems[i]);
    }


    /* For each set, find connections to all set neighbors. */
    edges_list = (int **) smalloc_ret((unsigned) nsets_tot * sizeof(int *));
    if (edges_list == NULL) goto skip;
    for (set = 0; set < nsets_tot-1; set++) {
	edges_list[set] = NULL;
    }

    ewgts_list = (int **) smalloc_ret((unsigned) nsets_tot * sizeof(int *));
    if (ewgts_list == NULL) goto skip;
    for (set = 0; set < nsets_tot-1; set++) {
	ewgts_list[set] = NULL;
    }

    nedges = (int *) smalloc_ret((unsigned) nsets_tot * sizeof(int));
    adj_sets = (int *) smalloc_ret((unsigned) nsets_tot * sizeof(int));
    if (nedges == NULL || adj_sets == NULL) goto skip;

    size = (int) (&(vtx_elems[1]) - &(vtx_elems[0]));
    ncomm = 0;
    ewgt = 1;
    nmax = 0;
    for (set = 0; set < nsets_tot-1; set++) {
	if (sizes[set] > nmax) nmax = sizes[set];
        for (i = 0; i < nsets_tot; i++) {
	    adj_sets[i] = 0;
        }
	for (ptr = set_list[set].next; ptr != NULL; ptr = ptr->next) {
	    vertex = ((int) (ptr - vtx_elems)) / size;
	    for (j = 1; j < graph[vertex]->nedges; j++) {
		set2 = assign[graph[vertex]->edges[j]];
		if (using_ewgts)
		    ewgt = graph[vertex]->ewgts[j];
		adj_sets[set2] += ewgt;
	    }
	}

	/* Now save adj_sets data to later construct graph. */
	j = 0;
	for (i = set+1; i < nsets_tot; i++) {
	    if (adj_sets[i] != 0)
		j++;
	}
	nedges[set] = j;
	if (j) {
	    edges_list[set] = edges = (int *) smalloc_ret((unsigned) j * sizeof(int));
	    ewgts_list[set] = ewgts = (int *) smalloc_ret((unsigned) j * sizeof(int));
	    if (edges == NULL || ewgts == NULL) goto skip;
	}
	j = 0;
	for (i = set + 1; i < nsets_tot; i++) {
	    if (adj_sets[i] != 0) {
		edges[j] = i;
		ewgts[j] = adj_sets[i];
		j++;
	    }
	}
	ncomm += j;
    }

    sfree((char *) adj_sets);
    adj_sets = NULL;


    /* Now compact all communication weight information into single */
    /* vector for sorting. */

    pairs = (struct ipairs *) smalloc_ret((unsigned) (ncomm + 1) * sizeof(struct ipairs));
    comm_vals = (double *) smalloc_ret((unsigned) (ncomm + 1) * sizeof(double));
    if (pairs == NULL || comm_vals == NULL) goto skip;

    j = 0;
    for (set = 0; set < nsets_tot - 1; set++) {
	eptr = edges_list[set];
	ewptr = ewgts_list[set];
	for (k = 0; k < nedges[set]; k++) {
	    set2 = eptr[k];
	    pairs[j].val1 = set;
	    pairs[j].val2 = set2;
	    comm_vals[j] = ewptr[k];
	    j++;
	}
    }

    sfree((char *) nedges);
    nedges = NULL;

    indices = (int *) smalloc_ret((unsigned) (ncomm + 1) * sizeof(int));
    space = (int *) smalloc_ret((unsigned) (ncomm + 1) * sizeof(int));
    if (indices == NULL || space == NULL) goto skip;

    mergesort(comm_vals, ncomm, indices, space);
    sfree((char *) space);
    sfree((char *) comm_vals);
    space = NULL;
    comm_vals = NULL;

    for (set = 0; set < nsets_tot - 1; set++) {
	if (edges_list[set] != NULL)
	    sfree((char *) edges_list[set]);
	if (ewgts_list[set] != NULL)
	    sfree((char *) ewgts_list[set]);
    }
    sfree((char *) ewgts_list);
    sfree((char *) edges_list);
    ewgts_list = NULL;
    edges_list = NULL;

    /* 2 for 2 subsets, 20 for safety margin. Should check this at run time. */
    nmax = 2 * nmax + 20;

    subgraph = (struct vtx_data **) smalloc_ret((unsigned) (nmax + 1) * sizeof(struct vtx_data *));
    degrees = (short *) smalloc_ret((unsigned) (nmax + 1) * sizeof(short));
    glob2loc = (int *) smalloc_ret((unsigned) (nvtxs + 1) * sizeof(int));
    loc2glob = (int *) smalloc_ret((unsigned) (nmax + 1) * sizeof(int));
    sub_assign = (short *) smalloc_ret((unsigned) (nmax + 1) * sizeof(short));
    old_sub_assign = (short *) smalloc_ret((unsigned) (nmax + 1) * sizeof(short));

    if (subgraph == NULL || degrees == NULL || glob2loc == NULL ||
        loc2glob == NULL || sub_assign == NULL || old_sub_assign == NULL) {
	goto skip;
    }

    if (TERM_PROP) {
        term_wgts[1] = (float *) smalloc_ret((unsigned) (nmax + 1) * sizeof(float));
	if (term_wgts[1] == NULL) goto skip;
    }
    else {
	term_wgts[1] = NULL;
    }

    /* Do large boundaries first to encourage convergence. */
    any_change = FALSE;
    for (i = ncomm - 1; i >= 0; i--) {
	j = indices[i];
	set1 = pairs[j].val1;
	set2 = pairs[j].val2;


	/* Make sure subgraphs aren't too big. */
	if (sizes[set1] + sizes[set2] > nmax) {
	    nmax = sizes[set1] + sizes[set2];

	    temp = srealloc_ret((char *) subgraph,
		(unsigned) (nmax + 1) * sizeof(struct vtx_data *));
	    if (temp == NULL) {
		goto skip;
	    }
	    else {
		subgraph = (struct vtx_data **) temp;
	    }

	    temp = srealloc_ret((char *) degrees,
		(unsigned) (nmax + 1) * sizeof(short));
	    if (temp == NULL) {
		goto skip;
	    }
	    else {
		degrees = (short *) temp;
	    }

	    temp = srealloc_ret((char *) loc2glob,
		(unsigned) (nmax + 1) * sizeof(int));
	    if (temp == NULL) {
		goto skip;
	    }
	    else {
		loc2glob = (int *) temp;
	    }

	    temp = srealloc_ret((char *) sub_assign,
		(unsigned) (nmax + 1) * sizeof(short));
	    if (temp == NULL) {
		goto skip;
	    }
	    else {
		sub_assign = (short *) temp;
	    }

	    temp = srealloc_ret((char *) old_sub_assign,
		(unsigned) (nmax + 1) * sizeof(short));
	    if (temp == NULL) {
		goto skip;
	    }
	    else {
		old_sub_assign = (short *) temp;
	    }

	    if (TERM_PROP) {
	        temp = srealloc_ret((char *) term_wgts[1],
		    (unsigned) (nmax + 1) * sizeof(float));
	        if (temp == NULL) {
		    goto skip;
	        }
	        else {
		    term_wgts[1] = (float *) temp;
	        }
	    }
	}


	if (TERM_PROP) {
	    if (architecture == 0) {
		j = set1 ^ set2;
		dist = 0;
		while (j) {
		    if (j & 1) dist++;
		    j >>= 1;
		}
	    }
	    else if (architecture > 0) {
		dist = abs((set1 % mesh_dims[0]) - (set2 % mesh_dims[0]));
		dist += abs(((set1 / mesh_dims[0]) % mesh_dims[1]) -
		            ((set2 / mesh_dims[0]) % mesh_dims[1]));
		dist += abs((set1 / (mesh_dims[0] * mesh_dims[1])) -
		            (set2 / (mesh_dims[0] * mesh_dims[1])));
	    }
	    hops[0][1] = hops[1][0] = (short) dist;
	}

	change = kl_refine(graph, subgraph, set_list, vtx_elems, assign,
	    set1, set2, glob2loc, loc2glob, sub_assign, old_sub_assign,
	    degrees, using_ewgts, hops, goal, sizes, term_wgts, architecture,
	    mesh_dims);
       
	any_change |= change;
    }
    error = 0;

skip:
    if (error) {
	strout("\nWARNING: No space to refine partition.");
	strout("         NO PARTITION REFINEMENT PERFORMED.\n");
    }

    if (edges_list != NULL) {
        for (set = 0; set < nsets_tot - 1; set++) {
	    if (edges_list[set] != NULL)
		sfree((char *) edges_list[set]);
	}
        sfree((char *) edges_list);
    }

    if (ewgts_list != NULL) {
        for (set = 0; set < nsets_tot - 1; set++) {
	    if (ewgts_list[set] != NULL)
	        sfree((char *) ewgts_list[set]);
	}
        sfree((char *) ewgts_list);
    }

    sfree((char *) space);
    sfree((char *) comm_vals);
    sfree((char *) nedges);
    sfree((char *) adj_sets);
    sfree((char *) term_wgts[1]);
    sfree((char *) old_sub_assign);
    sfree((char *) sub_assign);
    sfree((char *) loc2glob);
    sfree((char *) glob2loc);
    sfree((char *) degrees);
    sfree((char *) subgraph);

    sfree((char *) indices);
    sfree((char *) pairs);

    sfree((char *) sizes);
    sfree((char *) vtx_elems);
    sfree((char *) set_list);

    return(any_change);
}
