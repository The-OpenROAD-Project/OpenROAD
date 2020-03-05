/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include <string.h>
#include "params.h"
#include "defs.h"
#include "structs.h"


void      sequence(graph, nvtxs, nedges, using_ewgts, vwsqrt,
		             solver_flag, rqi_flag, vmax, eigtol)
struct vtx_data **graph;	/* graph data structure */
int       nvtxs;		/* number of vertices in graph */
int       nedges;		/* number of edges in graph */
int       using_ewgts;		/* are edge weights being used? */
double   *vwsqrt;		/* sqrt of vertex weights (length nvtxs+1) */
int       solver_flag;		/* which eigensolver should I use? */
int       rqi_flag;		/* use multilevel eigensolver? */
int       vmax;			/* if so, how many vtxs to coarsen down to? */
double    eigtol;		/* tolerance on eigenvectors */
{
    extern char SEQ_FILENAME[];	/* name of sequence file */
    extern int RQI_CONVERGENCE_MODE;	/* residual or partition mode? */
    extern int LANCZOS_CONVERGENCE_MODE;	/* residual or partition mode? */
    extern double sequence_time;/* time spent sequencing */
    struct vtx_data **subgraph;	/* subgraph data structure */
    struct edgeslist *edge_list;/* edges added for connectivity */
    double   *yvecs[MAXDIMS + 1];	/* space for pointing to eigenvectors */
    double    evals[MAXDIMS + 1];	/* corresponding eigenvalues */
    double   *subvwsqrt;	/* vwsqrt vector for subgraphs */
    double   *values;		/* sorted Fiedler vector values */
    double   *subvals;		/* values for one connected component */
    double    goal[2];		/* needed for eigen convergence mode = 1 */
    double    total_vwgt;	/* sum of all vertex weights */
    double    time;		/* time spent sequencing */
    float    *term_wgts[2];	/* dummy vector for terminal weights */
    int      *setsize;		/* size of each connected component */
    int      *glob2loc;		/* maps graph vtxs to subgraph vtxs */
    int      *loc2glob;		/* maps subgraph vtxs to graph vtxs */
    int      *permutation;	/* computed permutation */
    int      *subperm;		/* partial permutation */
    short    *compnum;		/* component number for each vertex */
    short    *degree;		/* degrees of vertices in subgraph */
    double    maxdeg;		/* maximum weighted degree of a vertex */
    int       using_vwgts;	/* are vertex weights being used? */
    int       subnvtxs;		/* number of vertices in subgraph */
    int       subnedges;	/* number of edges in subgraph */
    int       maxsize;		/* size of largest connected component */
    int       subvwgt_max;	/* largest vertex weight in component */
    int       ncomps;		/* number of connected components */
    int       comp;		/* loops over connected components */
    int       nused;		/* number of vertices already handled */
    int       old_rqi_conv_mode;/* value of RQI_CONVERGENCE_MODE */
    int       old_lan_conv_mode;/* value of LANCZOS_CONVERGENCE_MODE */
    int       i;		/* loop counters */
    int      *space;
    FILE     *orderfile;
    void      mergesort(), eigensolve(), free_edgeslist(), y2x();
    void      make_subvector(), make_subgraph(), remake_graph();
    void      make_maps2();
    double    find_maxdeg(), *smalloc(), seconds();
    int       find_edges(), sfree();

    time = seconds();
    using_vwgts = (vwsqrt != NULL);

    /* Sort each connected component seperately. */
    compnum = (short *) smalloc((unsigned) (nvtxs + 1) * sizeof(short));
    permutation = (int *) smalloc((unsigned) nvtxs * sizeof(int));
    values = (double *) smalloc((unsigned) (nvtxs + 1) * sizeof(double));

    space = (int *) smalloc((unsigned) nvtxs * sizeof(int));
    ncomps = find_edges(graph, nvtxs, compnum, space, &edge_list);
    ++ncomps;

    free_edgeslist(edge_list);
    yvecs[1] = (double *) smalloc((unsigned) (nvtxs + 1) * sizeof(double));

    term_wgts[1] = NULL;
    subvwsqrt = NULL;

    old_rqi_conv_mode = RQI_CONVERGENCE_MODE;
    old_lan_conv_mode = LANCZOS_CONVERGENCE_MODE;

    RQI_CONVERGENCE_MODE = 0;
    LANCZOS_CONVERGENCE_MODE = 0;

    maxsize = nvtxs;
    if (ncomps > 1) {

	/* Find size of largest set. */
	setsize = (int *) smalloc((unsigned) ncomps * sizeof(int));
	for (comp = 0; comp < ncomps; comp++)
	    setsize[comp] = 0;
	for (i = 1; i <= nvtxs; i++)
	    ++setsize[compnum[i]];
	maxsize = 0;
	for (comp = 0; comp < ncomps; comp++)
	    if (setsize[comp] > maxsize)
		maxsize = setsize[comp];

	glob2loc = (int *) smalloc((unsigned) (nvtxs + 1) * sizeof(int));
	loc2glob = (int *) smalloc((unsigned) (maxsize + 1) * sizeof(int));
	subgraph = (struct vtx_data **) smalloc((unsigned)
					(maxsize + 1) * sizeof(struct vtx_data *));
	degree = (short *) smalloc((unsigned) (maxsize + 1) * sizeof(short));
	if (using_vwgts) {
	    subvwsqrt = (double *) smalloc((unsigned) (maxsize + 1) * sizeof(double));
	}
    }

    nused = 0;

    for (comp = 0; comp < ncomps; comp++) {
	subperm = &(permutation[nused]);
	subvals = &(values[nused]);

	if (ncomps > 1) {
	    make_maps2(compnum, nvtxs, comp, glob2loc, loc2glob);
	    subnvtxs = setsize[comp];
	    make_subgraph(graph, subgraph, subnvtxs, &subnedges, compnum, comp,
			  glob2loc, loc2glob, degree, using_ewgts);

	    if (using_vwgts) {
		make_subvector(vwsqrt, subvwsqrt, subnvtxs, loc2glob);
	    }
	}
	else {
	    subgraph = graph;
	    subnvtxs = nvtxs;
	    subnedges = nedges;
	    subvwsqrt = vwsqrt;
	}

	if (using_vwgts) {
	    subvwgt_max = 0;
	    total_vwgt = 0;
	    for (i = 1; i <= subnvtxs; i++) {
		if (subgraph[i]->vwgt > subvwgt_max)
		    subvwgt_max = subgraph[i]->vwgt;
		total_vwgt += subgraph[i]->vwgt;
	    }
	}
	else {
	    subvwgt_max = 1;
	    total_vwgt = subnvtxs;
	}

	goal[0] = goal[1] = total_vwgt / 2;

	if (subnvtxs == 1) {
	    yvecs[1][1] = 0;
	}
	else {
	    maxdeg = find_maxdeg(subgraph, subnvtxs, using_ewgts, (float *) NULL);

	    eigensolve(subgraph, subnvtxs, subnedges, maxdeg, subvwgt_max, subvwsqrt,
		       using_vwgts, using_ewgts, term_wgts, 0, (float **) NULL,
		       yvecs, evals, 0, (short *) space, goal,
		       solver_flag, rqi_flag, vmax, 1, 3, eigtol);

	    /* Sort values in eigenvector */
	    if (using_vwgts) {
		y2x(yvecs, 1, subnvtxs, subvwsqrt);
	    }
	    mergesort(&(yvecs[1][1]), subnvtxs, subperm, space);
	}

	if (ncomps > 1) {
	    remake_graph(subgraph, subnvtxs, loc2glob, degree, using_ewgts);
	    for (i = 0; i < subnvtxs; i++) {
		subvals[i] = yvecs[1][subperm[i] + 1];
		subperm[i] = loc2glob[subperm[i] + 1];
	    }
	}
	else {
	    for (i = 0; i < nvtxs; i++) {
		++subperm[i];
	    }
	}

	nused += subnvtxs;
    }

    /* Turn off timer. */
    sequence_time += seconds() - time;

    /* Print the ordering to a file. */
    /* Note that permutation is 1<->n. */
    orderfile = fopen(SEQ_FILENAME, "w");
    if (ncomps == 1) {
	for (i = 0; i < nvtxs; i++) {
	    fprintf(orderfile, "%-7d   %9.6f\n", permutation[i],
		    yvecs[1][permutation[i]]);
	}
    }
    else {
	for (i = 0; i < nvtxs; i++) {
	    fprintf(orderfile, "%-7d   %9.6f\n", permutation[i], values[i]);
	}
    }
    fclose(orderfile);

    RQI_CONVERGENCE_MODE = old_rqi_conv_mode;
    LANCZOS_CONVERGENCE_MODE = old_lan_conv_mode;

    if (ncomps > 1) {
	sfree((char *) degree);
	sfree((char *) subgraph);
	sfree((char *) loc2glob);
	sfree((char *) glob2loc);
	sfree((char *) setsize);
	if (using_vwgts)
	    sfree((char *) subvwsqrt);
    }

    sfree((char *) yvecs[1]);
    sfree((char *) space);
    sfree((char *) values);
    sfree((char *) permutation);
    sfree((char *) compnum);

    printf("%d connected components found\n", ncomps);
    printf("Sequence printed to file `%s'\n", SEQ_FILENAME);
}
