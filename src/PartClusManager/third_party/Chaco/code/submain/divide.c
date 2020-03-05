/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include "params.h"
#include "defs.h"
#include "structs.h"

void      divide(graph, nvtxs, nedges, using_vwgts, using_ewgts, vwsqrt,
		           igeom, coords, assignment, goal,
		           architecture, term_wgts,
		           global_method, local_method, rqi_flag, vmax, ndims,
		           eigtol, hop_mtx, nsets, striping)
struct vtx_data **graph;	/* graph data structure */
int       nvtxs;		/* number of vertices in graph */
int       nedges;		/* number of edges in graph */
int       using_vwgts;		/* are vertex weights being used? */
int       using_ewgts;		/* are edge weights being used? */
double   *vwsqrt;		/* sqrt of vertex weights (length nvtxs+1) */
int       igeom;		/* geometry dimension for inertial method */
float   **coords;		/* coordinates for inertial method */
short    *assignment;		/* set number of each vtx (length n) */
double   *goal;			/* desired set sizes */
int       architecture;		/* 0 => hypercube, d => d-dimensional mesh */
float    *term_wgts[];		/* weights for terminal propogation */
int       global_method;	/* global partitioning algorithm */
int       local_method;		/* local partitioning algorithm */
int       rqi_flag;		/* should I use multilevel eigensolver? */
int       vmax;			/* if so, # vertices to coarsen down to */
int       ndims;		/* number of eigenvectors */
double    eigtol;		/* tolerance on eigenvectors */
short     (*hop_mtx)[MAXSETS];	/* between-set hop costs for KL */
int       nsets;		/* number of sets to partition into */
int       striping;		/* partition by striping into pieces? */
{
    extern int DEBUG_TRACE;	/* trace main execution path? */
    extern int DEBUG_CONNECTED;	/* debug flag for connected components */
    extern int DEBUG_KL;	/* debug flag for KL variants */
    extern int COARSE_NLEVEL_KL;/* how often to invoke KL while uncoarsening */
    extern int MAPPING_TYPE;	/* how to map from eigenvectors to partition */
    extern int LANCZOS_TYPE;	/* which Lanczos variant to use */
    extern int MAKE_CONNECTED;	/* force connectivity in spectral method? */
    extern double KL_IMBALANCE;	/* fractional imbalance allowed in KL */
    extern int VERTEX_SEPARATOR;/* vertex instead of edge separator? */
    extern int VERTEX_COVER;	/* form/improve vtx separator via matching? */
    extern int COARSE_BPM;	/* apply matching/flow while uncoarsening? */
    struct connect_data *cdata;	/* data for enforcing connectivity */
    double   *yvecs[MAXDIMS + 1];	/* space for pointing to eigenvectors */
    double    evals[MAXDIMS + 1];	/* corresponding eigenvalues */
    double    weights[MAXSETS];	/* vertex weight in each set */
    double    maxdeg;		/* maximum weighted degree of a vertex */
    double    total_weight;	/* weight of all vertices */
    double    temp_goal[2];	/* goal to simulate bisection while striping */
    double   *fake_goal;	/* either goal or temp_goal */
    int      *active;		/* keeping track of vtxs in BFS (length nvtxs) */
    int      *bndy_list;	/* list of boundary vtxs */
    int      *null_ptr;		/* argument to klspiff */
    short    *mark;		/* for marking vtxs in BFS (length nvtxs+1) */
    int       vwgt_max;		/* largest vertex weight */
    int       max_dev;		/* largest allowed deviation from balance */
    int       mediantype;	/* how to map from eigenvectors to partition */
    int       solver_flag;	/* which Lanczos variant to use */
    int       mkconnected;	/* enforce connectivity in spectral method? */
    int       i;		/* loop counters */
    int       simple_type;	/* which type of simple partitioning to use */
    double   *smalloc();
    int       sfree(), find_bndy(), find_side_bndy();
    void      make_connected(), print_connected(), coarsen_kl(), eigensolve();
    void      coarsen_klv(), klvspiff();
    void      assign(), make_unconnected(), inertial(), klspiff(), simple_part();
    void      count_weights(), bpm_improve(), countup_vtx_sep();
    double    find_maxdeg();

    if (DEBUG_TRACE > 0) {
	printf("<Entering divide, nvtxs = %d, nedges = %d>\n", nvtxs, nedges);
    }

    if (nvtxs <= 0) return;
    if (nedges == 0 && global_method != 3) {
	global_method = 4;
	local_method = 2;
    }

    bndy_list = NULL;

    if (striping) {
	ndims = 1;
	mediantype = 4;

	temp_goal[0] = temp_goal[1] = 0;
	for (i = 0; 2 * i + 1 < nsets; i++) {
	    temp_goal[0] += goal[i];
	    temp_goal[1] += goal[nsets - 1 - i];
	}
	i = nsets / 2;
	if (2 * i != nsets) {
	    temp_goal[0] += .5 * goal[i];
	    temp_goal[1] += .5 * goal[i];
	}
	fake_goal = temp_goal;
    }
    else {
	mediantype = MAPPING_TYPE;
	fake_goal = goal;
    }

    if (using_vwgts) {
	vwgt_max = 0;
	for (i = 1; i <= nvtxs; i++) {
	    if (graph[i]->vwgt > vwgt_max)
		vwgt_max = graph[i]->vwgt;
	}
    }
    else
	vwgt_max = 1;

    /* Perform one of the global partitionings on this sub-graph. */
    if (global_method == 1) {	/* Multilevel method. */
	mkconnected = MAKE_CONNECTED;
	solver_flag = LANCZOS_TYPE;

	if (VERTEX_SEPARATOR) {
	    coarsen_klv(graph, nvtxs, nedges, using_vwgts, using_ewgts, term_wgts,
			igeom, coords, 
			vwgt_max, assignment, goal, architecture, hop_mtx,
			solver_flag, ndims, nsets, vmax, mediantype, mkconnected,
			eigtol, COARSE_NLEVEL_KL, 0, &bndy_list, weights, FALSE);
	    if (VERTEX_COVER && !COARSE_BPM) {
		max_dev = vwgt_max;
		total_weight = 0;
		for (i = 0; i < nsets; i++) {
		    total_weight += goal[i];
		}
		total_weight *= KL_IMBALANCE / nsets;
		if (total_weight > max_dev) {
		    max_dev = total_weight;
		}
		bpm_improve(graph, assignment, goal, max_dev, &bndy_list,
			    weights, using_vwgts);
	    }
	}
	else {
	    coarsen_kl(graph, nvtxs, nedges, using_vwgts, using_ewgts, term_wgts,
		       igeom, coords, vwgt_max, assignment, goal, architecture, hop_mtx,
		       solver_flag, ndims, nsets, vmax, mediantype, mkconnected,
		       eigtol, COARSE_NLEVEL_KL, 0, &bndy_list, weights, FALSE);

	    if (VERTEX_COVER) {
		max_dev = vwgt_max;
		total_weight = 0;
		for (i = 0; i < nsets; i++) {
		    total_weight += goal[i];
		}
		total_weight *= KL_IMBALANCE / nsets;
		if (total_weight > max_dev) {
		    max_dev = total_weight;
		}
		if (goal[0] - weights[0] <= goal[1] - weights[1])
		    i = 0;
		else
		    i = 1;
		find_side_bndy(graph, nvtxs, assignment, i, 2, &bndy_list);
		count_weights(graph, nvtxs, assignment, nsets + 1,
			      weights, using_vwgts);
		if (DEBUG_KL > 0) {
		    printf("After KL, before bpm_improve\n");
		    countup_vtx_sep(graph, nvtxs, assignment);
		}
		bpm_improve(graph, assignment, goal, max_dev, &bndy_list,
			    weights, using_vwgts);
	    }

	}
    }

    else if (global_method == 2) {	/* Spectral method. */
	mkconnected = MAKE_CONNECTED;
	solver_flag = LANCZOS_TYPE;

	active = (int *) smalloc((unsigned) nvtxs * sizeof(int));
	for (i = 1; i <= ndims; i++) {
	    yvecs[i] = (double *) smalloc((unsigned) (nvtxs + 1) * sizeof(double));
	}

	if (mkconnected) {
	    /* If doing multi-level KL, this happens on coarse graph. */
	    mark = (short *) &(yvecs[1][0]);
	    make_connected(graph, nvtxs, &nedges, mark, active, &cdata, using_ewgts);
	    if (DEBUG_CONNECTED > 0) {
		printf("Enforcing connectivity\n");
		print_connected(cdata);
	    }
	}
	else if (DEBUG_CONNECTED > 0) {
	    printf("Not enforcing connectivity\n");
	}

	maxdeg = find_maxdeg(graph, nvtxs, using_ewgts, (float *) NULL);
	eigensolve(graph, nvtxs, nedges, maxdeg, vwgt_max, vwsqrt,
		   using_vwgts, using_ewgts, term_wgts, igeom, coords,
		   yvecs, evals, architecture,
		   assignment, fake_goal,
		   solver_flag, rqi_flag, vmax, ndims, mediantype, eigtol);

	assign(graph, yvecs, nvtxs, ndims, architecture, nsets, vwsqrt,
	       assignment, active, mediantype, goal, vwgt_max);

	for (i = 1; i <= ndims; i++)
	    sfree((char *) yvecs[i]);

	if (mkconnected) {
	    make_unconnected(graph, &nedges, &cdata, using_ewgts);
	}

	sfree((char *) active);
    }

    else if (global_method == 3) {	/* Inertial method. */
	inertial(graph, nvtxs, architecture, nsets, igeom, coords, assignment,
		 goal, using_vwgts);
    }

    else if (global_method == 4) {	/* Linear ordering. */
	simple_type = 3;
	simple_part(graph, nvtxs, assignment, nsets, simple_type, goal);
    }

    else if (global_method == 5) {	/* Random partitioning. */
	simple_type = 2;
	simple_part(graph, nvtxs, assignment, nsets, simple_type, goal);
    }

    else if (global_method == 6) {	/* Scattered partitioning. */
	simple_type = 1;
	simple_part(graph, nvtxs, assignment, nsets, simple_type, goal);
    }

    /* Perform a local refinement, if specified, on the global partitioning. */
    if (local_method == 1 && global_method != 1) {
	/* If global_method == 1, already did KL as part of multilevel-KL. */
	null_ptr = NULL;
	max_dev = vwgt_max;
	total_weight = 0;
	for (i = 0; i < nsets; i++) {
	    total_weight += goal[i];
	}
	total_weight *= KL_IMBALANCE / nsets;
	if (total_weight > max_dev) {
	    max_dev = total_weight;
	}
	if (VERTEX_SEPARATOR) {
	    find_bndy(graph, nvtxs, assignment, 2, &bndy_list);
	    count_weights(graph, nvtxs, assignment, nsets + 1, weights,
			  (vwgt_max != 1));
	    klvspiff(graph, nvtxs, assignment, goal,
		     max_dev, &bndy_list, weights);
	    if (VERTEX_COVER) {
		bpm_improve(graph, assignment, goal, max_dev, &bndy_list,
			    weights, using_vwgts);
	    }
	}
	else {
	    if (global_method != 2) {
		maxdeg = find_maxdeg(graph, nvtxs, using_ewgts, (float *) NULL);
	    }
	    count_weights(graph, nvtxs, assignment, nsets, weights,
			  (vwgt_max != 1));
	    klspiff(graph, nvtxs, assignment, nsets, hop_mtx, goal, term_wgts,
		    max_dev, maxdeg, using_ewgts, &null_ptr, weights);
	    if (VERTEX_COVER) {
		if (goal[0] - weights[0] <= goal[1] - weights[1])
		    i = 0;
		else
		    i = 1;
		find_side_bndy(graph, nvtxs, assignment, i, 2, &bndy_list);
		count_weights(graph, nvtxs, assignment, nsets + 1, weights, (vwgt_max != 1));
		if (DEBUG_KL > 0) {
		    printf("After KL, before bpm_improve\n");
		    countup_vtx_sep(graph, nvtxs, assignment);
		}
		bpm_improve(graph, assignment, goal, max_dev, &bndy_list,
			    weights, using_vwgts);
	    }
	}
    }
    else if (global_method != 1 && VERTEX_COVER) {
	find_bndy(graph, nvtxs, assignment, 2, &bndy_list);
	count_weights(graph, nvtxs, assignment, nsets + 1, weights, (vwgt_max != 1));
	if (DEBUG_KL > 0) {
	    printf("Before bpm_improve\n");
	    countup_vtx_sep(graph, nvtxs, assignment);
	}
	max_dev = vwgt_max;
	total_weight = 0;
	for (i = 0; i < nsets; i++) {
	    total_weight += goal[i];
	}
	total_weight *= KL_IMBALANCE / nsets;
	if (total_weight > max_dev) {
	    max_dev = total_weight;
	}
	bpm_improve(graph, assignment, goal, max_dev, &bndy_list, weights, 
		    using_vwgts);
    }

    if (bndy_list != NULL) {
	sfree((char *) bndy_list);
    }
}
