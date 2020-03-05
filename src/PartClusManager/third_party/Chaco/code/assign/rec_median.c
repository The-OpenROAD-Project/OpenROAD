/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"structs.h"
#include	"params.h"
#include	"defs.h"

/* Recursively apply median to a SINGLE vector of values */

void      rec_median_1(graph, vals, nvtxs, active, cube_or_mesh, nsets,
		                 goal, using_vwgts, assign, top)
struct vtx_data **graph;	/* data structure with vertex weights */
double   *vals;			/* values of which to find median */
int       nvtxs;		/* number of values I own */
int      *active;		/* space for list of nmyvals ints */
int       cube_or_mesh;		/* 0=> hypercube, other=> mesh */
int       nsets;		/* number of sets to divide into */
double   *goal;			/* desired sizes for sets */
int       using_vwgts;		/* are vertex weights being used? */
short    *assign;		/* set each vertex gets assigned to */
int       top;			/* is this the top call in the recursion? */
{
    struct vtx_data **sub_graph;/* subgraph data structure with vertex weights */
    double   *sub_vals;		/* subgraph entries in vals vector */
    double    merged_goal[MAXSETS / 2];	/* combined goal values */
    double    sub_vwgt_sum;	/* sum of vertex weights in subgraph */
    int      *loc2glob;		/* mapping from subgraph to graph numbering */
    int       mapping[MAXSETS];	/* appropriate set number */
    short    *sub_assign;	/* assignment returned from subgraph */
    int       sub_nvtxs;	/* number of vertices in subgraph */
    int       sub_nsets;	/* number of sets in side of first partition */
    int       setsize[2];	/* number of vertices in two subsets */
    int       maxsize;		/* number of vertices in larger subset */
    int       ndims;		/* number of bits defining set */
    int       mesh_dims[3];	/* shape of fictitious mesh */
    int       i, j;		/* loop counters */
    double   *smalloc();
    int       bit_reverse(), sfree();
    void      median(), make_subgoal(), make_subvector();
    void      make_maps2();

    cube_or_mesh = (cube_or_mesh != 0);
    mesh_dims[1] = mesh_dims[2] = 1;

    if (!cube_or_mesh) {
	for (i = 0; i < 2; i++) {
	    merged_goal[i] = 0;
	    for (j = i; j < nsets; j += 2)
		merged_goal[i] += goal[j];
	}
    }
    else {
	merged_goal[0] = merged_goal[1] = 0;
	for (i = 0; i < (nsets + 1) / 2; i++)
	    merged_goal[0] += goal[i];
	for (i = (nsets + 1) / 2; i < nsets; i++)
	    merged_goal[1] += goal[i];
    }

    median(graph, vals, nvtxs, active, merged_goal, using_vwgts, assign);

    if (nsets > 2) {
	/* Find size of largest subproblem. */
	setsize[0] = setsize[1] = 0;
	for (i = 1; i <= nvtxs; i++)
	    ++setsize[assign[i]];
	maxsize = max(setsize[0], setsize[1]);

	sub_assign = (short *) smalloc((unsigned) (maxsize + 1) * sizeof(short));
	sub_vals = (double *) smalloc((unsigned) (maxsize + 1) * sizeof(double));
	loc2glob = (int *) smalloc((unsigned) (maxsize + 1) * sizeof(int));
	if (!using_vwgts)
	    sub_graph = NULL;
	else
	    sub_graph = (struct vtx_data **)
	       smalloc((unsigned) (maxsize + 1) * sizeof(struct vtx_data *));

	for (i = 0; i < 2; i++) {
	    /* Construct subproblem. */
	    if (i == 0) {
		sub_nsets = (nsets + 1) / 2;
	    }
	    else {
		sub_nsets = nsets / 2;
	    }
	    sub_nvtxs = setsize[i];

	    for (j = 1; j <= sub_nvtxs; j++)
		sub_assign[j] = 0;
	    make_maps2(assign, nvtxs, i, (int *) NULL, loc2glob);

	    if (sub_nsets > 1) {

		if (!using_vwgts) {
		    sub_vwgt_sum = sub_nvtxs;
		}
		else {
		    sub_vwgt_sum = 0;
		    for (j = 1; j <= sub_nvtxs; j++) {
			sub_graph[j] = graph[loc2glob[j]];
			sub_vwgt_sum += sub_graph[j]->vwgt;
		    }
		}

		make_subvector(vals, sub_vals, sub_nvtxs, loc2glob);

		mesh_dims[0] = nsets;
		make_subgoal(goal, merged_goal, 2, cube_or_mesh, nsets, mesh_dims,
			     i, sub_vwgt_sum);

		rec_median_1(sub_graph, sub_vals, sub_nvtxs, active, cube_or_mesh,
			     sub_nsets, merged_goal, using_vwgts,
			     sub_assign, FALSE);
	    }
	    /* Merge subassignment with current assignment. */
	    for (j = 1; j <= sub_nvtxs; j++) {
		assign[loc2glob[j]] |= (sub_assign[j] << 1);
	    }
	}

	/* Now I have the set striped in bit reversed order. */
	if (top) {
	    ndims = 0;
	    for (i = 1; i < nsets; i <<= 1)
		ndims++;

	    for (i = 0; i < nsets; i++)
		mapping[i] = bit_reverse(i, ndims);

	    for (i = 1; i <= nvtxs; i++)
		assign[i] = mapping[assign[i]];
	}

	if (sub_graph != NULL)
	    sfree((char *) sub_graph);
	sfree((char *) loc2glob);
	sfree((char *) sub_vals);
	sfree((char *) sub_assign);
    }
}



/* Recursively apply median to a SEVERAL vectors of values */
/* Divide with first, and use result to divide with second, etc. */
/* Note: currently only works for power-of-two number of processors. */

void      rec_median_k(graph, vals, nvtxs, active, ndims, cube_or_mesh,
		                 goal, using_vwgts, assign)
struct vtx_data **graph;	/* data structure with vertex weights */
double  **vals;			/* values of which to find median */
int       nvtxs;		/* number of values I own */
int      *active;		/* space for list of nmyvals ints */
int       ndims;		/* number of dimensions to divide */
int       cube_or_mesh;		/* 0 => hypercube, d => d-dimensional mesh */
double   *goal;			/* desired sizes for sets */
int       using_vwgts;		/* are vertex weights being used? */
short    *assign;		/* set each vertex gets assigned to */
{
    struct vtx_data **sub_graph;/* subgraph data structure with vertex weights */
    double   *sub_vals[MAXDIMS];/* subgraph entries in vals vectors */
    double    merged_goal[MAXSETS / 2];	/* combined goal values */
    double    sub_vwgt_sum;	/* sum of vertex weights in subgraph */
    int      *loc2glob;		/* mapping from subgraph to graph numbering */
    short    *sub_assign;	/* assignment returned from subgraph */
    int       sub_nvtxs;	/* number of vertices in subgraph */
    int       setsize[2];	/* number of vertices in two subsets */
    int       maxsize;		/* number of vertices in larger subset */
    int       nsets;		/* number of sets we are dividing into */
    int       mesh_dims[3];	/* shape of fictitious mesh */
    int       i, j;		/* loop counters */
    double   *smalloc();
    int       sfree();
    void      median(), make_subgoal(), make_subvector(), make_maps2();

    mesh_dims[1] = mesh_dims[2] = 1;

    /* Note: This is HYPERCUBE/MESH dependent.  We'll want to combine the */
    /* sizes of different sets on the different architectures. */
    nsets = 1 << ndims;
    for (i = 0; i < 2; i++) {
	merged_goal[i] = 0;
	for (j = i; j < nsets; j += 2)
	    merged_goal[i] += goal[j];
    }

    median(graph, vals[1], nvtxs, active, merged_goal, using_vwgts, assign);

    if (ndims > 1) {
	/* Find size of largest subproblem. */
	setsize[0] = setsize[1] = 0;
	for (i = 1; i <= nvtxs; i++)
	    ++setsize[assign[i]];
	maxsize = max(setsize[0], setsize[1]);

	sub_assign = (short *) smalloc((unsigned) (maxsize + 1) * sizeof(short));
	for (i = 1; i < ndims; i++) {
	    sub_vals[i] = (double *) smalloc((unsigned) (maxsize + 1) * sizeof(double));
	}
	loc2glob = (int *) smalloc((unsigned) (maxsize + 1) * sizeof(int));
	if (!using_vwgts)
	    sub_graph = NULL;
	else
	    sub_graph = (struct vtx_data **)
	       smalloc((unsigned) (maxsize + 1) * sizeof(struct vtx_data *));

	for (i = 0; i < 2; i++) {
	    /* Construct subproblem. */

	    sub_nvtxs = setsize[i];

	    for (j = 1; j <= sub_nvtxs; j++)
		sub_assign[j] = 0;

	    make_maps2(assign, nvtxs, i, (int *) NULL, loc2glob);

	    if (!using_vwgts)
		sub_vwgt_sum = sub_nvtxs;
	    else {
		sub_vwgt_sum = 0;
		for (j = 1; j <= sub_nvtxs; j++) {
		    sub_graph[j] = graph[loc2glob[j]];
		    sub_vwgt_sum += sub_graph[j]->vwgt;
		}
	    }

	    for (j = 2; j <= ndims; j++) {
		make_subvector(vals[j], sub_vals[j - 1], sub_nvtxs, loc2glob);
	    }

	    mesh_dims[0] = nsets;
	    make_subgoal(goal, merged_goal, 2, cube_or_mesh, nsets, mesh_dims,
			 i, sub_vwgt_sum);

	    rec_median_k(sub_graph, sub_vals, sub_nvtxs, active, ndims - 1,
			 cube_or_mesh, merged_goal, using_vwgts,
			 sub_assign);

	    /* Merge subassignment with current assignment. */
	    for (j = 1; j <= sub_nvtxs; j++) {
		assign[loc2glob[j]] |= (sub_assign[j] << 1);
	    }
	}

	if (sub_graph != NULL)
	    sfree((char *) sub_graph);
	sfree((char *) loc2glob);
	for (i = 1; i < ndims; i++)
	    sfree((char *) sub_vals[i]);
	sfree((char *) sub_assign);
    }
}
