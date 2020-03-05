/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	<math.h>
#include	"defs.h"
#include	"structs.h"

/* Perform Rayleigh Quotient Iteration */

void      rqi(A, yvecs, index, n, r1, r2, v, w, x, y, work, tol, initshift, evalest,
	      vwsqrt, orthlist, cube_or_mesh, nsets, assignment, active, mediantype,
	      goal, vwgt_max, ndims)
struct vtx_data **A;		/* matrix/graph being analyzed */
double  **yvecs;		/* eigenvectors to be refined */
int       index;		/* index of vector in yvecs to be refined */
double   *r1, *r2, *v, *w, *x, *y, *work;	/* work space for symmlq */
int       n;			/* number of rows/columns in matrix */
double    tol;			/* error tolerance in eigenpair */
double    initshift;		/* initial shift */
double   *evalest;		/* returned eigenvalue */
double   *vwsqrt;		/* square roots of vertex weights */
struct orthlink *orthlist;	/* lower evecs to orthogonalize against */
int       cube_or_mesh;		/* 0 => hypercube, d => d-dimensional mesh */
int       nsets;		/* number of sets to divide into */
short    *assignment;		/* set number of each vtx (length n+1) */
int      *active;		/* space for nvtxs integers */
int       mediantype;		/* which partitioning strategy to use */
double   *goal;			/* desired set sizes */
int       vwgt_max;		/* largest vertex weight */
int       ndims;		/* dimensionality of partition */
{
    extern int DEBUG_EVECS;	/* debug flag for eigen computation */
    extern int DEBUG_TRACE;	/* trace main execution path */
    extern int WARNING_EVECS;	/* warning flag for eigen computation */
    extern int RQI_CONVERGENCE_MODE;	/* type of convergence monitoring to do */
    int       rqisteps;		/* # rqi rqisteps */
    double    res;		/* convergence quant for rqi */
    double    last_res;		/* res on previous rqi step */
    double    macheps;		/* machine precision calculated by symmlq */
    double    normxlim;		/* a stopping criteria for symmlq */
    double    normx;		/* norm of the solution vector */
    int       symmlqitns;	/* # symmlq itns */
    int       inv_it_steps;	/* intial steps of inverse iteration */
    long      itnmin;		/* symmlq input */
    double    shift, rtol;	/* symmlq input */
    long      precon, goodb, nout;	/* symmlq input */
    long      checka, intlim;	/* symmlq input */
    double    anorm, acond;	/* symmlq output */
    double    rnorm, ynorm;	/* symmlq output */
    long      istop, itn;	/* symmlq output */
    long      long_n;		/* copy of n for passing to symmlq */
    int       warning;		/* warning on possible misconvergence */
    double    factor;		/* ratio between previous res and new tol */
    double    minfactor;	/* minimum acceptable value of factor */
    int       converged;	/* has process converged yet? */
    double   *u;		/* name of vector being refined */
    short    *old_assignment;	/* previous assignment vector */
    short    *assgn_pntr;	/* pntr to assignment vector */
    short    *old_assgn_pntr;	/* pntr to previous assignment vector */
    int       assigndiff;	/* discrepancies between old and new assignment */
    int       assigntol;	/* tolerance on convergence of assignment vector */
    int       first;		/* is this the first RQI step? */
    int       i;		/* loop index */

    double    dot(), norm();
    int       symmlq_();
    void      splarax(), scadd(), vecscale(), doubleout(), assign(), x2y(), strout();
    double   *smalloc();
    int       sfree();

    if (DEBUG_TRACE > 0) {
	printf("<Entering rqi>\n");
    }

    /* Initialize RQI loop */
    u = yvecs[index];
    splarax(y, A, n, u, vwsqrt, r1);
    shift = dot(u, 1, n, y);
    scadd(y, 1, n, -shift, u);
    res = norm(y, 1, n);	/* eigen-residual */
    rqisteps = 0;		/* a counter */
    symmlqitns = 0;		/* a counter */

    /* Set invariant symmlq parameters */
    precon = FALSE;		/* FALSE until we figure out a good way */
    goodb = TRUE;		/* should be TRUE for this application */
    nout = 0;			/* set to 0 for no Symmlq output; 6 for lots */
    checka = FALSE;		/* if don't know by now, too bad */
    intlim = n;			/* set to enforce a maximum number of Symmlq itns */
    itnmin = 0;			/* set to enforce a minimum number of Symmlq itns */
    long_n = n;			/* type change for alint */

    if (DEBUG_EVECS > 0) {
	printf("Using RQI/Symmlq refinement on graph with %d vertices.\n", n);
    }
    if (DEBUG_EVECS > 1) {
	printf("  step      lambda est.            Ares          Symmlq its.   istop  factor  delta\n");
	printf("    0");
	doubleout(shift, 1);
	doubleout(res, 1);
	printf("\n");
    }

    if (RQI_CONVERGENCE_MODE == 1) {
	assigntol = tol * n;
	old_assignment = (short *) smalloc((unsigned) (n + 1) * sizeof(short));
    }

    /* Perform RQI */
    inv_it_steps = 2;
    warning = FALSE;
    factor = 10;
    minfactor = factor / 2;
    first = TRUE;
    if (res < tol)
	converged = TRUE;
    else
	converged = FALSE;
    while (!converged) {
	if (res / tol < 1.2) {
	    factor = max(factor / 2, minfactor);
	}
	rtol = res / factor;

	/* exit Symmlq if iterate is this large */
	normxlim = 1.0 / rtol;

	if (rqisteps < inv_it_steps) {
	    shift = initshift;
	}

	symmlq_(&long_n, &u[1], &r1[1], &r2[1], &v[1], &w[1], &x[1], &y[1],
		work, &checka, &goodb, &precon, &shift, &nout,
		&intlim, &rtol, &istop, &itn, &anorm, &acond,
		&rnorm, &ynorm, (double *) A, vwsqrt, (double *) orthlist,
		&macheps, &normxlim, &itnmin);
	symmlqitns += itn;
	normx = norm(x, 1, n);
	vecscale(u, 1, n, 1.0 / normx, x);
	splarax(y, A, n, u, vwsqrt, r1);
	shift = dot(u, 1, n, y);
	scadd(y, 1, n, -shift, u);
	last_res = res;
	res = norm(y, 1, n);
	if (res > last_res) {
	    warning = TRUE;
	}
	rqisteps++;

	if (res < tol)
	    converged = TRUE;

	if (RQI_CONVERGENCE_MODE == 1 && !converged && ndims == 1) {
	    if (first) {
		assign(A, yvecs, n, 1, cube_or_mesh, nsets, vwsqrt, assignment,
		       active, mediantype, goal, vwgt_max);
		x2y(yvecs, ndims, n, vwsqrt);
		first = FALSE;
		assigndiff = n;	/* dummy value for debug chart */
	    }
	    else {
		/* copy assignment to old_assignment */
		assgn_pntr = assignment;
		old_assgn_pntr = old_assignment;
		for (i = n + 1; i; i--) {
		    *old_assgn_pntr++ = *assgn_pntr++;
		}

		assign(A, yvecs, n, ndims, cube_or_mesh, nsets, vwsqrt, assignment,
		       active, mediantype, goal, vwgt_max);
		x2y(yvecs, ndims, n, vwsqrt);

		/* count differences in assignment */
		assigndiff = 0;
		assgn_pntr = assignment;
		old_assgn_pntr = old_assignment;
		for (i = n + 1; i; i--) {
		    if (*old_assgn_pntr++ != *assgn_pntr++)
			assigndiff++;
		}
		assigndiff = min(assigndiff, n - assigndiff);
		if (assigndiff <= assigntol)
		    converged = TRUE;
	    }
	}

	if (DEBUG_EVECS > 1) {
	    printf("   %2d", rqisteps);
	    doubleout(shift, 1);
	    doubleout(res, 1);
	    printf("     %3ld", itn);
	    printf("          %ld", istop);
	    printf("      %g", factor);
	    if (RQI_CONVERGENCE_MODE == 1)
		printf("     %d\n", assigndiff);
	    else
		printf("\n");
	}
    }
    *evalest = shift;

    if (WARNING_EVECS > 0 && warning) {
	strout("WARNING: Residual convergence not monotonic; RQI may have misconverged.\n");
    }

    if (DEBUG_EVECS > 0) {
	printf("Eval ");
	doubleout(*evalest, 1);
	printf("   RQI steps %d,  Symmlq iterations %d.\n\n", rqisteps, symmlqitns);
    }

    if (RQI_CONVERGENCE_MODE == 1) {
	sfree((char *) old_assignment);
    }
}
