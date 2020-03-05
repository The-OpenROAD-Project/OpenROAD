/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include <math.h>
#include "structs.h"
#include "defs.h"
#include "params.h"

/* These comments are for version 1 of selective orthogonalization:
   This version of selective orthogonalization largely follows that described in
   Pareltt and Scott, "The Lanczos Algorithm with Selective Orthogonalization",
   Math Comp v33 #145, 1979. Different heuristics are used to control loss of
   orthogonality. Specifically, the pauses to check for convergence of Ritz pairs
   at first come at a small but increasing interval as the computation builds up.
   Later they come at a regular, pre-set interval, e.g. every 10 steps. Hence this
   is similar to Grear's periodic reorthogonalization scheme, but with an adaptive
   period. A small number of ritz pairs at both ends of the spectrum are monitored
   for convergence at each pause. The number monitored gradually increases as more
   Ritz pairs converge so that the number monitored at each pause is always at least
   2 greater than the number that have previously converged. This is a fairly
   conservative strategy, at least in the context of the typical Laplacian graph
   matrices studied in connection with load balancing. But it may occasionally
   result in premature loss of orthogonality. If the loss of orthogonality is
   severe, Lanczos may fail to meet the set eigen tolerance or mis-converge to
   the wrong eigenpair. These conditions will usually be detected by the algorithm
   and a warning issued. If the graph is small, full orthogonalization is
   then a fall back option. If the graph is large, one of the multi-level schemes
   should be tried. The algorithm orthogonalizes the starting vector and each 
   residual vector against the vector of all ones since we know that is an 
   eigenvector of the Laplacian (which we don't want to see).  This is 
   accomplished through calls to orthog1. These can be removed, but then
   we must compute an extra eigenvalue and the vector of all ones shows up as
   a Ritz pair we need to orthogonalize against in order to maintain the basis.
   This is more expensive then simply taking it out of the residual on each step,
   and doing the latter also reduces the number of iterations. The Ritz values
   being monitored at each step are computed using the QL algorithm or bisection
   on the Sturm sequence, whichever is faster based on a simple complexity model.
   In practice the time spent computing Ritz values is only a small portion of
   the total Lanczos time, so more complex schemes based on interpolating the
   bottom pivot function represent only a very marginal savings in execution
   time at the expense of either some robustness or substantially increased
   code complexity. */

/* These comments are for version 2 of selective orthogonalization:
   These comments are cumulative from version 1. Modified the heuristics to
   make it faster. Now only checks left end of spectrum for converging Ritz pairs.
   Only checking the left end of the spectrum for converging Ritz pairs is in principle
   not as good as checking both ends, but in practice seems to be no worse and usually 
   better than checking both ends. Since the distribution of eigenvalues for Laplacian
   graphs of interest seems to generally result in much more rapid convergence on the
   right end of the spectrum, ignoring the converging Ritz pairs there saves a lot of
   work. */

/* These comments are for version 3 of selective orthogonalization: 
   This option hasn't performed that well, so I've rescinded it from the menu. 
   Uses Paige suggestion of monitoring dot product of current Lanczos vector
   against first to sense loss of orthogonality. This can save time by avoiding
   calls to the QL or bisection routines for finding Ritz values. */

/* These comments explain some of indexing conventions used:
   Note that the indexing of the tridiagonal matrix is consistent with Parlett and
   Scott's Math Comp  '79 paper "The Lanczos Algorithm With Selective Orthogonalization"
   and with Golub and Van Loan's (2nd ed.) treatment in Chapter 9. So on iteration j
   the tridiagonal matrix T has diagonal = alpha[1], alpha[2] ... alpha[j] and
   off-diagonal = beta[1], beta[2] ... beta[j-1]. beta[0] is the norm of the initial
   residual and beta[j] is used in monitoring convergence. */

/* These comments pertain to the calculation of the eigenvalues of T:
   There are several safety features in the code which computes the Ritz values.
   Computation is first attempted using either the QL algorithm or Sturm sequence
   bisection, whichever is predicted to be cheaper based on simple complexity analysis 
   (normally bisection is cheaper in this context). The classic bisection algorithm
   can fail due to overflow, so a re-scaling heuristic is used to guard against this.
   The number of bisection and QL iterations is monitored and if a reasonable maximum
   is observed these routines abort. If either routine fails, the other is applied
   to the problem. If both routines fail, Lanczos is backed up to the last pause point
   and an approximate eigenvector is computed based on the available information. */

/* These comments pertain to the calculation of eigenvectors of T:
   In the first instance the eigenvectors are computed using a bidirectional
   recurrence which amounts to one fancy step of inverse iteration. If this
   fails to achieve the requested tolerance, a more expensive version of inverse 
   iteration (Tinvit from Eispack) is tried. The better of the two answers is 
   then used. See the routine Tevec for more comments and details. */
    
void      lanczos_SO(A, n, d, y, lambda, bound, eigtol, vwsqrt, maxdeg, version,
               cube_or_mesh, nsets, assignment, active, mediantype, goal, vwgt_max)
struct vtx_data **A;		/* sparse matrix in row linked list format */
int       n;			/* problem size */
int       d;			/* problem dimension = number of eigvecs to find */
double  **y;			/* columns of y are eigenvectors of A  */
double   *lambda;		/* ritz approximation to eigenvals of A */
double   *bound;		/* on ritz pair approximations to eig pairs of A */
double    eigtol;		/* tolerance on eigenvectors */
double   *vwsqrt;		/* square roots of vertex weights */
double    maxdeg;		/* maximum degree of graph */
int       version;		/* flags which version of sel. orth. to use */
int       cube_or_mesh;		/* 0 => hypercube, d => d-dimensional mesh */
int       nsets;		/* number of sets to divide into */
short    *assignment;		/* set number of each vtx (length n+1) */
int      *active;		/* space for nvtxs integers */
int       mediantype;		/* which partitioning strategy to use */
double   *goal;			/* desired set sizes */
int       vwgt_max;		/* largest vertex weight */
{
    extern FILE *Output_File;		/* output file or null */
    extern int LANCZOS_SO_INTERVAL;	/* interval between orthogonalizations */
    extern int LANCZOS_CONVERGENCE_MODE;/* type of Lanczos convergence test */
    extern int LANCZOS_MAXITNS;		/* maximum Lanczos iterations allowed */
    extern int DEBUG_EVECS;		/* print debugging output? */
    extern int DEBUG_TRACE;		/* trace main execution path */
    extern int WARNING_EVECS;		/* print warning messages? */
    extern double BISECTION_SAFETY;     /* safety factor for bisection alg. */
    extern double SRESTOL;		/* resid tol for T evec comp */
    extern double DOUBLE_EPSILON;	/* machine precision */
    extern double DOUBLE_MAX;	/* largest double value */
    extern double splarax_time;	/* time matvec */
    extern double orthog_time;	/* time orthogonalization work */
    extern double evec_time;	/* time to generate eigenvectors */
    extern double ql_time;	/* time tridiagonal eigenvalue work */
    extern double blas_time;	/* time for blas. linear algebra */
    extern double init_time;	/* time to allocate, intialize variables */
    extern double scan_time;	/* time for scanning eval and bound lists */
    extern double debug_time;	/* time for (some of) debug computations */
    extern double ritz_time;	/* time to generate ritz vectors */
    extern double pause_time;	/* time to compute whether to pause */
    double bis_safety;		/* real safety factor for bisection alg. */
    int       i, j, k;		/* indicies */
    int       maxj;		/* maximum number of Lanczos iterations */
    double   *u, *r;		/* Lanczos vectors */
    double   *alpha, *beta;	/* the Lanczos scalars from each step */
    double   *ritz;		/* copy of alpha for ql */
    double   *workj;		/* work vector, e.g. copy of beta for ql */
    double   *workn;		/* work vector, e.g. product Av for checkeig */
    double   *s;		/* eigenvector of T */
    double  **q;		/* columns of q are Lanczos basis vectors */
    double   *bj;		/* beta(j)*(last el. of corr. eigvec s of T) */
    double    Sres;		/* how well Tevec calculated eigvec s */
    double    Sres_max;		/* Max value of Sres */
    int       inc_bis_safety;   /* need to increase bisection safety */
    double   *Ares;		/* how well Lanczos calc. eigpair lambda,y */
    int      *index;		/* the Ritz index of an eigenpair */
    struct orthlink **solist;	/* vec. of structs with vecs. to orthog. against */
    struct scanlink *scanlist;	/* linked list of fields to do with min ritz vals */
    struct scanlink *curlnk;	/* for traversing the scanlist */
    double    bji_tol;		/* tol on bji est. of eigen residual of A */
    int       converged;	/* has the iteration converged? */
    double    goodtol;		/* error tolerance for a good Ritz vector */
    int       ngood;		/* total number of good Ritz pairs at current step */
    int       maxngood;		/* biggest val of ngood through current step */
    int       left_ngood;	/* number of good Ritz pairs on left end */
    int       right_ngood;	/* number of good Ritz pairs on right end */
    int       lastpause;	/* Most recent step with good ritz vecs */
    int       firstpause;	/* Is this the first pause? */
    int       nopauses;		/* Have there been any pauses? */
    int       interval;		/* number of steps between pauses */
    double    time;		/* Current clock time */
    int       left_goodlim;	/* number of ritz pairs checked on left end */
    int       right_goodlim;	/* number of ritz pairs checked on right end */
    double    Anorm;		/* Norm estimate of the Laplacian matrix */
    int       pausemode;	/* which Lanczos pausing criterion to use */
    int       pause;		/* whether to pause */
    int       temp;		/* used to prevent redundant index computations */
    short    *old_assignment;	/* set # of each vtx on previous pause, length n+1 */ 
    short    *assgn_pntr;	/* pntr to assignment vector */
    short    *old_assgn_pntr;	/* pntr to previous assignment vector */
    int       assigndiff;	/* # of differences between old and new assignment */
    int       assigntol;	/* tolerance on convergence of assignment vector */
    int       ritzval_flag;	/* status flag for get_ritzvals() */
    int       memory_ok;	/* True until lanczos runs out of memory */

    struct orthlink *makeorthlnk();	/* makes space for new entry in orthog. set */
    struct scanlink *mkscanlist();	/* makes initial scan list for min ritz vecs */
    double   *mkvec();		/* allocates space for a vector, dies if problem */
    double   *mkvec_ret();	/* allocates space for a vector, returns error code */
    double   *smalloc();	/* safe version of malloc */
    double    dot();		/* standard dot product routine */
    double    norm();		/* vector norm */
    double    Tevec();		/* calc eigenvector of T by linear recurrence */
    double    checkeig();	/* calculate residual of eigenvector of A */
    double    lanc_seconds();	/* switcheable timer */
    int       sfree(); 		/* free allocated memory safely */
    int       lanpause(); 	/* figure when to pause Lanczos iteration */
    int       get_ritzvals(); 	/* compute eigenvalues of T */
    void      assign();		/* generate a set assignment from eigenvectors */
    void      setvec();		/* initialize a vector */
    void      vecscale();	/* scale a vector */
    void      splarax();	/* matrix vector multiply */
    void      update();		/* add a scalar multiple of a vector to another */
    void      sorthog();	/* orthogonalize a vector against a list of others */
    void      bail();		/* our exit routine */
    void      scanmin();	/* find small values in vector, store in linked list */
    void      frvec();		/* free vector */
    void      scadd();		/* add scalar multiple of vector to another */
    void      orthog1();	/* efficiently orthogonalize against vector of ones */
    void      vecran();		/* fill vector with random entries */
    void      solistout();	/* print out orthogonalization list */
    void      doubleout();	/* print a double precision number */
    void      orthogvec();	/* orthogonalize one vector against another */
    void      warnings();	/* post various warnings about computation */
    void      mkeigvecs();	/* assemble eigenvectors */
    void      strout();		/* print string to screen and output file */

	
    if (DEBUG_TRACE > 0) {
	printf("<Entering lanczos_so>\n");
    }

    if (DEBUG_EVECS > 0)  {
	printf("Selective orthogonalization Lanczos (v. %d), matrix size = %d.\n", version, n);
    }

    /* Initialize time. */
    time = lanc_seconds();

    if (n < d + 1) {
	bail("ERROR: System too small for number of eigenvalues requested.",1);
	/* d+1 since don't use zero eigenvalue pair */
    }

    /* Allocate space. */
    maxj = LANCZOS_MAXITNS;
    u = mkvec(1, n);
    r = mkvec(1, n);
    workn = mkvec(1, n);
    Ares = mkvec(0, d);
    index = (int *) smalloc((unsigned) (d + 1) * sizeof(int));
    alpha = mkvec(1, maxj);
    beta = mkvec(0, maxj);
    ritz = mkvec(1, maxj);
    s = mkvec(1, maxj);
    bj = mkvec(1, maxj);
    workj = mkvec(0, maxj);
    q = (double **) smalloc((unsigned) (maxj + 1) * sizeof(double *));
    solist = (struct orthlink **) smalloc((unsigned) (maxj + 1) * sizeof(struct orthlink *));
    scanlist = mkscanlist(d);
    if (LANCZOS_CONVERGENCE_MODE == 1) {
	old_assignment = (short *) smalloc((unsigned) (n + 1) * sizeof(short));
    }

    /* Set some constants governing the orthogonalization heuristic. */
    ngood = 0;
    maxngood = 0;
    bji_tol = eigtol;
    assigntol = eigtol * n;
    Anorm = 2 * maxdeg;				/* Gershgorin estimate for ||A|| */
    goodtol = Anorm * sqrt(DOUBLE_EPSILON);	/* Parlett & Scott's bound, p.224 */
    interval = 2 + (int) min(LANCZOS_SO_INTERVAL - 2, n / (2 * LANCZOS_SO_INTERVAL));
    bis_safety = BISECTION_SAFETY;

    if (DEBUG_EVECS > 0) {
	printf("  eigtol %g\n", eigtol);
	printf("  maxdeg %g\n", maxdeg);
	printf("  goodtol %g\n", goodtol);
	printf("  interval %d\n", interval);
	printf("  maxj %d\n", maxj);
	printf("  srestol %g\n", SRESTOL);
	if (LANCZOS_CONVERGENCE_MODE == 1)
	    printf("  assigntol %d\n", assigntol);
    }

    /* Initialize space. */
    vecran(r, 1, n);
    if (vwsqrt == NULL) {
	orthog1(r, 1, n);
    }
    else {
	orthogvec(r, 1, n, vwsqrt);
    }
    beta[0] = norm(r, 1, n);
    q[0] = mkvec(1, n);
    setvec(q[0], 1, n, 0.0);
    setvec(bj, 1, maxj, DOUBLE_MAX);

    /* Main Lanczos loop. */
    j = 1;
    lastpause = 0;
    pausemode = 1;
    left_ngood = 0;
    right_ngood = 0;
    left_goodlim = 0;
    right_goodlim = 0;
    converged = FALSE;
    Sres_max = 0.0;
    inc_bis_safety = FALSE;
    ritzval_flag = 0;
    memory_ok = TRUE;
    firstpause = FALSE;
    nopauses = TRUE;
    init_time += lanc_seconds() - time;
    while ((j <= maxj) && (!converged) && (ritzval_flag == 0) && memory_ok) {
	time = lanc_seconds();

	/* Allocate next Lanczos vector. If fail, back up to last pause. */
	q[j] = mkvec_ret(1, n);
        if (q[j] == NULL) {
	    memory_ok = FALSE;
  	    if (DEBUG_EVECS > 0 || WARNING_EVECS > 0) {
                strout("WARNING: Lanczos out of memory; computing best approximation available.\n");
            }
	    if (nopauses) {
	        bail("ERROR: Sorry, can't salvage Lanczos.",1); 
  	        /* ... save yourselves, men.  */
	    }
    	    for (i = lastpause+1; i <= j-1; i++) {
	        frvec(q[i], 1);
    	    }
            j = lastpause;
	}

	/* Basic Lanczos iteration */
	vecscale(q[j], 1, n, 1.0 / beta[j - 1], r);
	blas_time += lanc_seconds() - time;
	time = lanc_seconds();
	splarax(u, A, n, q[j], vwsqrt, workn);
	splarax_time += lanc_seconds() - time;
	time = lanc_seconds();
	update(r, 1, n, u, -beta[j - 1], q[j - 1]);
	alpha[j] = dot(r, 1, n, q[j]);
	update(r, 1, n, r, -alpha[j], q[j]);
	blas_time += lanc_seconds() - time;

	/* Selective orthogonalization */
	time = lanc_seconds();
	if (vwsqrt == NULL) {
	    orthog1(r, 1, n);
	}
	else {
	    orthogvec(r, 1, n, vwsqrt);
	}
	if ((j == (lastpause + 1)) || (j == (lastpause + 2))) {
	    sorthog(r, n, solist, ngood);
	}
	orthog_time += lanc_seconds() - time;
	beta[j] = norm(r, 1, n);
	time = lanc_seconds();
	pause = lanpause(j, lastpause, interval, q, n, &pausemode, version, beta[j]);
	pause_time += lanc_seconds() - time;
	if (pause) {
	    nopauses = FALSE;
	    if (lastpause == 0)
		firstpause = TRUE;
	    else
		firstpause = FALSE;
	    lastpause = j;

	    /* Compute limits for checking Ritz pair convergence. */
	    if (version == 1) {
		if (left_ngood + 2 > left_goodlim) {
		    left_goodlim = left_ngood + 2;
		}
		if (right_ngood + 3 > right_goodlim) {
		    right_goodlim = right_ngood + 3;
		}
	    }
	    if (version == 2) {
		if (left_ngood + 2 > left_goodlim) {
		    left_goodlim = left_ngood + 2;
		}
		right_goodlim = 0;
	    }

	    /* Special case: need at least d Ritz vals on left. */
	    left_goodlim = max(left_goodlim, d);

	    /* Special case: can't find more than j total Ritz vals. */
	    if (left_goodlim + right_goodlim > j) {
		left_goodlim = min(left_goodlim, j);
		right_goodlim = j - left_goodlim;
	    }

	    /* Find Ritz vals using faster of Sturm bisection or QL. */
	    time = lanc_seconds();
	    blas_time += lanc_seconds() - time;
	    time = lanc_seconds();
	    if (inc_bis_safety) {
		bis_safety *= 10;
		inc_bis_safety = FALSE;
	    }
	    ritzval_flag = get_ritzvals(alpha, beta, j, Anorm, workj, 
			ritz, d, left_goodlim, right_goodlim, eigtol, bis_safety);
	    ql_time += lanc_seconds() - time;

	    /* If get_ritzvals() fails, back up to last pause point and exit main loop. */
            if (ritzval_flag != 0) {
                if (DEBUG_EVECS > 0 || WARNING_EVECS > 0) {
                    strout("ERROR: Lanczos failed in computing eigenvalues of T; computing");
                    strout("       best readily available approximation to eigenvector.\n");
                }
		if (firstpause) {
		    bail("ERROR: Sorry, can't salvage Lanczos.",1); 
		    /* ... save yourselves, men.  */
		}
    		for (i = lastpause+1; i <= j; i++) {
		    frvec(q[i], 1);
    		}
                j = lastpause;
                get_ritzvals(alpha, beta, j, Anorm, workj,
                        ritz, d, left_goodlim, right_goodlim, eigtol, bis_safety);
            }

	    /* Scan for minimum evals of tridiagonal. */
	    time = lanc_seconds();
	    scanmin(ritz, 1, j, &scanlist);
	    scan_time += lanc_seconds() - time;

	    /* Compute Ritz pair bounds at left end. */
	    time = lanc_seconds();
	    setvec(bj, 1, j, 0.0);
	    for (i = 1; i <= left_goodlim; i++) {
		Sres = Tevec(alpha, beta - 1, j, ritz[i], s);
		if (Sres > Sres_max) {
		    Sres_max = Sres;
		}
		if (Sres > SRESTOL) {
		    inc_bis_safety = TRUE;
		}
		bj[i] = s[j] * beta[j];
		/* bj[i] = fabs(s[j] * beta[j]); if don't enforce in Tevec */
	    }

	    /* Compute Ritz pair bounds at right end. */
	    for (i = j; i > j - right_goodlim; i--) {
		Sres = Tevec(alpha, beta - 1, j, ritz[i], s);
		if (Sres > Sres_max) {
		    Sres_max = Sres;
		}
		if (Sres > SRESTOL) {
		    inc_bis_safety = TRUE;
		}
		bj[i] = s[j] * beta[j];
		/* bj[i] = fabs(s[j] * beta[j]); if don't enforce in Tevec */
	    }
	    ritz_time += lanc_seconds() - time;

	    /* Show the portion of the spectrum checked for convergence. */
	    if (DEBUG_EVECS > 2) {
		time = lanc_seconds();
		printf("index         Ritz vals            bji bounds   (j = %d)\n",j);
		for (i = 1; i <= left_goodlim; i++) {
		    printf("  %3d", i);
		    doubleout(ritz[i], 1);
		    doubleout(bj[i], 1);
		    printf("\n");
		}
		printf("\n");
		curlnk = scanlist;
		while (curlnk != NULL) {
		    temp = curlnk->indx;
		    if ((temp > left_goodlim) && (temp < j - right_goodlim)) {
			printf("  %3d", temp);
			doubleout(ritz[temp], 1);
			doubleout(bj[temp], 1);
			printf("\n");
		    }
		    curlnk = curlnk->pntr;
		}
		printf("\n");
		for (i = j - right_goodlim + 1; i <= j; i++) {
		    printf("  %3d", i);
		    doubleout(ritz[i], 1);
		    doubleout(bj[i], 1);
		    printf("\n");
		}
		printf("                            -------------------\n");
		printf("                goodtol:    %19.16f\n\n", goodtol);
		debug_time += lanc_seconds() - time;
	    }

	    /* Check for convergence. */
	    time = lanc_seconds();
	    if (LANCZOS_CONVERGENCE_MODE != 1 || d > 1) {
		/* check convergence of residual bound */
		converged = TRUE;
		if (j < d)
		    converged = FALSE;
		else {
		    curlnk = scanlist;
		    while (curlnk != NULL) {
			if (bj[curlnk->indx] > bji_tol) {
			    converged = FALSE;
			}
			curlnk = curlnk->pntr;
		    }
		}
	    }
	    if (LANCZOS_CONVERGENCE_MODE == 1 && d == 1) {
		/* check change in partition */
		if (firstpause) {
		    converged = TRUE;
		    if (j < d)
			converged = FALSE;
		    else {
			curlnk = scanlist;
			while (curlnk != NULL) {
			    if (bj[curlnk->indx] > bji_tol) {
				converged = FALSE;
			    }
			    curlnk = curlnk->pntr;
			}
		    }
		    if (!converged) {
		        /* compute current approx. to eigenvectors */
                        mkeigvecs(scanlist,lambda,bound,index,bj,d,&Sres_max,
                                  alpha,beta,j,s,y,n,q);

		        /* compute first assignment */
			assign(A, y, n, d, cube_or_mesh, nsets, vwsqrt, assignment,
			       active, mediantype, goal, vwgt_max);
		    }
		}
		else {
		    /* copy assignment to old_assignment */
		    assgn_pntr = assignment;
		    old_assgn_pntr = old_assignment;
		    for (i = n + 1; i; i--) {
			*old_assgn_pntr++ = *assgn_pntr++;
		    }

		    /* compute current approx. to eigenvectors */
                    mkeigvecs(scanlist,lambda,bound,index,bj,d,&Sres_max,
                              alpha,beta,j,s,y,n,q);

		    /* write new assignment */
		    assign(A, y, n, d, cube_or_mesh, nsets, vwsqrt, assignment,
			   active, mediantype, goal, vwgt_max);

		    assigndiff = 0;
		    assgn_pntr = assignment;
		    old_assgn_pntr = old_assignment;
		    for (i = n + 1; i; i--) {
			if (*old_assgn_pntr++ != *assgn_pntr++)
			    assigndiff++;
		    }
		    assigndiff = min(assigndiff, n - assigndiff);
		    if (DEBUG_EVECS > 1) {
			printf("  j %d,  change from last assignment %d\n\n", j, assigndiff);
		    }

		    if (assigndiff <= assigntol)
			converged = TRUE;
		    else
			converged = FALSE;
		}
	    }
	    scan_time += lanc_seconds() - time;

	    /* Show current estimates of evals and bounds (for help in tuning) */
	    if (DEBUG_EVECS > 2 && !converged) {
		time = lanc_seconds();

		/* compute current approx. to eigenvectors */
                mkeigvecs(scanlist,lambda,bound,index,bj,d,&Sres_max,
                          alpha,beta,j,s,y,n,q);

		/* Compute residuals and display associated info. */
		printf("j %4d;    lambda                Ares est.             Ares          index\n",j);
		for (i = 1; i <= d; i++) {
		    Ares[i] = checkeig(workn, A, y[i], n, lambda[i], vwsqrt, u);
		    printf("%2d.", i);
		    doubleout(lambda[i], 1);
		    doubleout(bound[i], 1);
		    doubleout(Ares[i], 1);
		    printf("   %3d\n", index[i]);
		}
		printf("\n");
		debug_time += lanc_seconds() - time;
	    }

	    if (!converged) {
		ngood = 0;
		left_ngood = 0;	/* for setting left_goodlim on next loop */
		right_ngood = 0;/* for setting right_goodlim on next loop */

		/* Compute converged Ritz pairs on left end */
		time = lanc_seconds();
		for (i = 1; i <= left_goodlim; i++) {
		    if (bj[i] <= goodtol) {
			ngood += 1;
			left_ngood += 1;
			if (ngood > maxngood) {
			    maxngood = ngood;
			    solist[ngood] = makeorthlnk();
			    (solist[ngood])->vec = mkvec(1, n);
			}
			(solist[ngood])->index = i;
			Sres = Tevec(alpha, beta - 1, j, ritz[i], s);
			if (Sres > Sres_max) {
			    Sres_max = Sres;
			}
			if (Sres > SRESTOL) {
			    inc_bis_safety = TRUE;
			}
			setvec((solist[ngood])->vec, 1, n, 0.0);
			for (k = 1; k <= j; k++) {
			    scadd((solist[ngood])->vec, 1, n, s[k], q[k]);
			}
		    }
		}

		/* Compute converged Ritz pairs on right end */
		for (i = j; i > j - right_goodlim; i--) {
		    if (bj[i] <= goodtol) {
			ngood += 1;
			right_ngood += 1;
			if (ngood > maxngood) {
			    maxngood = ngood;
			    solist[ngood] = makeorthlnk();
			    (solist[ngood])->vec = mkvec(1, n);
			}
			(solist[ngood])->index = i;
			Sres = Tevec(alpha, beta - 1, j, ritz[i], s);
			if (Sres > Sres_max) {
			    Sres_max = Sres;
			}
			if (Sres > SRESTOL) {
			    inc_bis_safety = TRUE;
			}
			setvec((solist[ngood])->vec, 1, n, 0.0);
			for (k = 1; k <= j; k++) {
			    scadd((solist[ngood])->vec, 1, n, s[k], q[k]);
			}
		    }
		}
		ritz_time += lanc_seconds() - time;

		if (DEBUG_EVECS > 2) {
		    time = lanc_seconds();
		    printf("  j %3d; goodlim lft %2d, rgt %2d; list ",
			   j, left_goodlim, right_goodlim);
		    solistout(solist, n, ngood, j);
		    printf("---------------------end of iteration---------------------\n\n");
		    debug_time += lanc_seconds() - time;
		}
	    }
	}
	j++;
    }
    j--;

    /* Collect eigenvalue and bound information. */
    time = lanc_seconds();
    mkeigvecs(scanlist,lambda,bound,index,bj,d,&Sres_max,alpha,beta,j,s,y,n,q);
    evec_time += lanc_seconds() - time;

    /* Analyze computation for and report additional problems */
    time = lanc_seconds();
    warnings(workn, A, y, n, lambda, vwsqrt, Ares, bound, index, 
             d, j, maxj, Sres_max, eigtol, u, Anorm, Output_File);
    debug_time += lanc_seconds() - time;

    /* free up memory */
    time = lanc_seconds();
    frvec(u, 1);
    frvec(r, 1);
    frvec(workn, 1);
    frvec(Ares, 0);
    sfree((char *) index);
    frvec(alpha, 1);
    frvec(beta, 0);
    frvec(ritz, 1);
    frvec(s, 1);
    frvec(bj, 1);
    frvec(workj, 0);
    for (i = 0; i <= j; i++) {
	frvec(q[i], 1);
    }
    sfree((char *) q);
    while (scanlist != NULL) {
	curlnk = scanlist->pntr;
	sfree((char *) scanlist);
	scanlist = curlnk;
    }
    for (i = 1; i <= maxngood; i++) {
	frvec((solist[i])->vec, 1);
	sfree((char *) solist[i]);
    }
    sfree((char *) solist);
    if (LANCZOS_CONVERGENCE_MODE == 1) {
	sfree((char *) old_assignment);
    }
    init_time += lanc_seconds() - time;
}
