/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include <math.h>
#include "params.h"
#include "defs.h"
#include "structs.h"

/* Invoke the eigenvector calculation */
void      eigensolve(graph, nvtxs, nedges, maxdeg, vwgt_max, vwsqrt,
		               using_vwgts, using_ewgts, term_wgts, igeom, coords,
		               yvecs, evals, architecture, assignment, goal,
	                 solver_flag, rqi_flag, vmax, ndims, mediantype, eigtol)
struct vtx_data **graph;	/* graph data structure */
int       nvtxs;		/* number of vertices in graph */
int       nedges;		/* number of edges in graph */
double    maxdeg;		/* largest (weighted) degree of a vertex */
int       vwgt_max;		/* largest vertex weight */
double   *vwsqrt;		/* sqrt of vertex weights (length nvtxs+1) */
int       using_vwgts;		/* are vertex weights being used? */
int       using_ewgts;		/* are edge weights being used? */
float    *term_wgts[];		/* terminal propagation weight vector */
int       igeom;		/* geometric dimensionality if given coords */
float   **coords;		/* coordinates of vertices */
double  **yvecs;		/* space for pointing to eigenvectors */
double   *evals;		/* eigenvalues associated with eigenvectors */
int       architecture;		/* 0 => hypercube, d => d-dimensional mesh */
short    *assignment;		/* set number of each vtx (length n+1) */
double   *goal;			/* desired set sizes */
int       solver_flag;		/* flag indicating which solver to use */
int       rqi_flag;		/* use multi-level techniques? */
int       vmax;			/* if so, how many vtxs to coarsen down to? */
int       ndims;		/* number of eigenvectors (2^d sets) */
int       mediantype;		/* which partitioning strategy to use */
double    eigtol;		/* tolerance on eigenvectors */
{
    extern int DEBUG_TRACE;	/* trace the execution of the code */
    extern int DEBUG_EVECS;	/* debug flag for eigenvector generation */
    extern int DEBUG_PERTURB;	/* debug flag for matrix perturbation */
    extern int PERTURB;		/* randomly perturb to break symmetry? */
    extern int NPERTURB;	/* number of edges to perturb */
    extern double PERTURB_MAX;	/* maximum size of perturbation */
    extern int COARSE_NLEVEL_RQI;	/* do RQI this often in uncoarsening */
    extern int LANCZOS_CONVERGENCE_MODE;/* how to stop Lanczos? */
    extern double lanczos_time;		/* time spent in Lanczos algorithm */
    extern double rqi_symmlq_time;	/* time spent in RQI/Symmlq method */
    extern int WARNING_EVECS;		/* print warning messages? */
    extern int LANCZOS_SO_PRECISION;	/* controls precision in eigen calc. */
    extern double SRESTOL;		/* resid tol for T evec computation */
    extern int LANCZOS_MAXITNS;         /* maximum Lanczos iterations allowed */
    extern int EXPERT;			/* user type */
    double    bound[MAXDIMS + 1];	/* ritz approx bounds to eigenpairs */
    double    time;		/* time marker */
    float    *dummy_twgt[2];	/* turns off terminal propagation */
    float    *twptr;		/* terminal propagation weight vector */
    int      *active;		/* space for nvtxs values */
    int       step;		/* current step in RQI counting */
    int       nstep;		/* number of uncoarsening levels between RQIs */
    int       version;		/* which version of sel. orth. to use */
    int       nsets;		/* number of sets to divide into */
    double   *g;		/* rhs n-vector in the extended eigenproblem */
    double   *ptr;		/* loops through yvec */
    double    w1, w2;		/* desired weights of two sets */
    double    term_tot;		/* sum of terminal weights */
    double    sigma;		/* norm constraint on extended eigenvector */
    int       i, j;		/* loop counter */
    int       normal;		/* use normal or extended eigensolver? */
    int       autoset_maxitns;	/* set LANCZOS_MAXITNS automatically? */ 
    int       prev_maxitns;	/* LANCZOS_MAXITNS value above this routine */ 
    int       autoset_srestol;	/* set SRESTOL automatically? */
    double    prev_srestol;	/* SRESTOL value above this routine */
    int       sfree();
    double    seconds(), *smalloc();
    void      coarsen(), lanczos_FO(), lanczos_SO(), vecout();
    void      lanczos_SO_float(), strout();
    void      perturb_init(), perturb_clear(), x2y(), y2x();
    int       lanczos_ext(), lanczos_ext_float();
FILE *fopen(), *file;


    if (DEBUG_TRACE > 0) {
	printf("<Entering eigensolve, nvtxs = %d, nedges = %d>\n", nvtxs, nedges);
    }

    if (nvtxs <= ndims) {		/* Pathological special case. */
	for (i = 1; i <= ndims; i++) {
	    for (j = 1; j <= nvtxs; j++) {
		yvecs[i][j] = 0;
	    }
	}
	return;
    }


    active = NULL;
    normal = FALSE;

    /* Autoset (if necessary) some parameters for the eigen calculation */
    autoset_maxitns = FALSE;
    autoset_srestol = FALSE;
    if (LANCZOS_MAXITNS < 0) {
	autoset_maxitns = TRUE;
	prev_maxitns = LANCZOS_MAXITNS;
	LANCZOS_MAXITNS = 2 * nvtxs;
    }
    if (SRESTOL < 0) {
	autoset_srestol = TRUE;
	prev_srestol = SRESTOL;
	SRESTOL = eigtol * eigtol;
    }

    /* Note: When (if ever) rqi_ext is done, change precedence of eigensolvers. */

    if (term_wgts[1] != NULL && ndims == 1) { /* then use lanczos_ext */
	if (PERTURB) {
	    if (NPERTURB > 0 && PERTURB_MAX > 0.0) {
		perturb_init(nvtxs);
		if (DEBUG_PERTURB > 0) {
		    printf("Matrix being perturbed with scale %e\n", PERTURB_MAX);
		}
	    }
	    else if (DEBUG_PERTURB > 0) {
		printf("Matrix not being perturbed\n");
	    }
	}

	version = 2;
	if (LANCZOS_CONVERGENCE_MODE == 1) {
	    active = (int *) smalloc((unsigned) nvtxs * sizeof(int));
	}
	nsets = 1 << ndims;

	w1 = goal[0];
	w2 = goal[1];
	sigma = sqrt(4*w1*w2/(w1+w2));
	g = (double *) smalloc((unsigned) (nvtxs+1)*sizeof(double));

	twptr = term_wgts[1];
	term_tot = 0;
	for (i=1; i<=nvtxs; i++) term_tot += twptr[i];
	term_tot /= (w1+w2);
	if (using_vwgts) {
	    for (i=1; i<=nvtxs; i++) {
		g[i] = twptr[i]/graph[i]->vwgt - term_tot;
	    }
	}
	else {
	    for (i=1; i<=nvtxs; i++) {
		g[i] = twptr[i] - term_tot;
	    }
	}

	time = seconds();

	if (LANCZOS_SO_PRECISION == 2) {	/* double precision */
	    normal = lanczos_ext(graph, nvtxs, ndims, yvecs, eigtol, vwsqrt,
	        maxdeg, version, g, sigma);
 	}
	else {					/* single precision */
	    normal = lanczos_ext_float(graph, nvtxs, ndims, yvecs, eigtol, vwsqrt,
	        maxdeg, version, g, sigma);
	}

	sfree((char *) g);
	if (active != NULL) sfree((char *) active);
	active = NULL;

	if (normal) {
	    if (WARNING_EVECS > 2) {
		strout("WARNING: Not an extended eigenproblem; switching to standard eigensolver.\n");
	    }
	}
	else {
	    if (w2 != w1) {
		if (using_vwgts) {
		    y2x(yvecs, ndims, nvtxs, vwsqrt);
		}
		sigma = (w2 - w1) / (w2 + w1);
		ptr = yvecs[1];
	        for (i=nvtxs; i; i--) {
		     *(++ptr) += sigma;
	        }
		/* Note: if assign() could skip scaling, next call unnecessary. */
		if (using_vwgts) {
		    x2y(yvecs, ndims, nvtxs, vwsqrt);
		}
	    }
	}

	if (PERTURB && NPERTURB > 0 && PERTURB_MAX > 0.0) {
	    perturb_clear();
	}

	lanczos_time += seconds() - time;
    }
    else {
	normal = TRUE;
    }

    if (normal) {
        if (rqi_flag) {
	    /* Solve using multi-level scheme RQI/Symmlq. */
	    time = seconds();
	    nstep = COARSE_NLEVEL_RQI;
	    step = 0;
	    dummy_twgt[1] = NULL;
	    coarsen(graph, nvtxs, nedges, using_vwgts, using_ewgts, dummy_twgt,
		    igeom, coords, yvecs, ndims, solver_flag, vmax, eigtol,
		    nstep, step, FALSE);

	    rqi_symmlq_time += seconds() - time;
	}

	else {			/* Use standard Lanczos. */
	    if (PERTURB) {
	        if (NPERTURB > 0 && PERTURB_MAX > 0.0) {
		    perturb_init(nvtxs);
		    if (DEBUG_PERTURB > 0) {
		        printf("Matrix being perturbed with scale %e\n", PERTURB_MAX);
		    }
	        }
	        else if (DEBUG_PERTURB > 0) {
		    printf("Matrix not being perturbed\n");
	        }
	    }

	    if (solver_flag == 1) {
		time = seconds();
		version = 1;
		lanczos_FO(graph, nvtxs, ndims, yvecs, evals, bound, eigtol,
		    vwsqrt, maxdeg, version);
		lanczos_time += seconds() - time;
	    }
	    if (solver_flag == 2) {
		time = seconds();
		version = 2;
		lanczos_FO(graph, nvtxs, ndims, yvecs, evals, bound, eigtol,
		    vwsqrt, maxdeg, version);
		lanczos_time += seconds() - time;
	    }
	    else if (solver_flag == 3) {
		version = 2;	/* orthog. against left end only */
		if (LANCZOS_CONVERGENCE_MODE == 1) {
		    active = (int *) smalloc((unsigned) nvtxs * sizeof(int));
		}
		nsets = 1 << ndims;
		time = seconds();
  		if (LANCZOS_SO_PRECISION == 2) {	/* double precision */
		    lanczos_SO(graph, nvtxs, ndims, yvecs, evals, bound,
			eigtol, vwsqrt, maxdeg, version, architecture, nsets,
			assignment, active, mediantype, goal, vwgt_max);
		}
  		else {					/* single precision */
		    lanczos_SO_float(graph, nvtxs, ndims, yvecs, evals, bound,
			eigtol, vwsqrt, maxdeg, version, architecture, nsets,
			assignment, active, mediantype, goal, vwgt_max);
		}
		lanczos_time += seconds() - time;
	    }
	    else if (solver_flag == 4) {
	 	if (EXPERT) {
		    version = 1; /* orthog. against both ends */
		}
		else {
		    /* this should have been caught earlier ... */
		    version = 2;
		    solver_flag = 3;
		}
		if (LANCZOS_CONVERGENCE_MODE == 1) {
		    active = (int *) smalloc((unsigned) nvtxs * sizeof(int));
		}
		nsets = 1 << ndims;
		time = seconds();
  		if (LANCZOS_SO_PRECISION == 1) {	/* Single precision */
		    lanczos_SO_float(graph, nvtxs, ndims, yvecs, evals, bound,
		        eigtol, vwsqrt, maxdeg, version, architecture, nsets,
			assignment, active, mediantype, goal, vwgt_max);
		}
  		else {				/* Double precision */
		    lanczos_SO(graph, nvtxs, ndims, yvecs, evals, bound,
			eigtol, vwsqrt, maxdeg, version, architecture, nsets,
			assignment, active, mediantype, goal, vwgt_max);
		}
		lanczos_time += seconds() - time;
	    }
	}

/*
file = fopen("CHACO.EVECS", "w");
for (i = 1; i <= nvtxs; i++) {
 for (j = 1; j <= ndims; j++) {
  fprintf(file, "%g ", (yvecs[j])[i]);
 }
  fprintf(file, "\n");
}
fclose(file);
*/


	if (PERTURB && NPERTURB > 0 && PERTURB_MAX > 0.0) {
	    perturb_clear();
	}
    }

    if (DEBUG_EVECS > 4) {
	for (nstep = 1; nstep <= ndims; nstep++) {
	    vecout(yvecs[nstep], 1, nvtxs, "Eigenvector", (char *) NULL);
	}
    }

    /* Auto-reset (if necessary) some parameters for the eigen calculation */
    if (autoset_maxitns) LANCZOS_MAXITNS = prev_maxitns;
    if (autoset_srestol) SRESTOL = prev_srestol;

    if (active != NULL)
	sfree((char *) active);

    if (DEBUG_TRACE > 1) {
	printf("<Leaving eigensolve>\n");
    }
}
