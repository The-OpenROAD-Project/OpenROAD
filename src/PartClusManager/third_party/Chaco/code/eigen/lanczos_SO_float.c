/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include <math.h>
#include "structs.h"
#include "defs.h"
#include "params.h"

/* See comments in lanczos_SO() */

void      lanczos_SO_float(A, n, d, y, lambda, bound, eigtol, vwsqrt, maxdeg, version,
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
    extern double BISECTION_SAFETY;	/* safety factor for T bisection */
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
    double bis_safety;		/* real safety factor for T bisection */
    int       i, j, k;		/* indicies */
    int       maxj;		/* maximum number of Lanczos iterations */
    float    *u, *r;		/* Lanczos vectors */
    double   *u_double;		/* double version of u */
    double   *alpha, *beta;	/* the Lanczos scalars from each step */
    double   *ritz;		/* copy of alpha for ql */
    double   *workj;		/* work vector, e.g. copy of beta for ql */
    float    *workn;		/* work vector, e.g. product Av for checkeig */
    double   *workn_double;	/* work vector, e.g. product Av for checkeig */
    double   *s;		/* eigenvector of T */
    float   **q;		/* columns of q are Lanczos basis vectors */
    double   *bj;		/* beta(j)*(last el. of corr. eigvec s of T) */
    double    Sres;		/* how well Tevec calculated eigvec s */
    double    Sres_max;		/* Max value of Sres */
    int       inc_bis_safety;	/* need to increase bisection safety */
    double   *Ares;		/* how well Lanczos calc. eigpair lambda,y */
    int      *index;		/* the Ritz index of an eigenpair */
    struct orthlink_float **solist;	/* vec. of structs with vecs. to orthog. against */
    struct scanlink *scanlist;	 	/* linked list of fields to do with min ritz vals */
    struct scanlink *curlnk;		/* for traversing the scanlist */
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
    float    *vwsqrt_float;	/* float version of vwsqrt */

    struct orthlink_float *makeorthlnk_float();	/* makes space for new entry in orthog. set */
    struct scanlink *mkscanlist();		/* makes initial scan list for min ritz vecs */
    double   *mkvec();		/* allocates space for a vector, dies if problem */
    float    *mkvec_float();	/* allocates space for a vector, dies if problem */
    float    *mkvec_ret_float();/* allocates space for a vector, returns error code */
    double   *smalloc();	/* safe version of malloc */
    double    dot_float();	/* standard dot product routine */
    double    norm_float();	/* vector norm */
    double    Tevec();		/* calc eigenvector of T by linear recurrence */
    double    checkeig();	/* calculate residual of eigenvector of A */
    double    lanc_seconds();	/* switcheable timer */
    int       sfree(); 		/* free allocated memory safely */
    int       lanpause_float();	/* figure when to pause Lanczos iteration */
    int       get_ritzvals(); 	/* compute eigenvalues of T */
    void      assign();		/* generate a set assignment from eigenvectors */
    void      setvec();		/* initialize a vector */
    void      setvec_float();	/* initialize a vector */
    void      vecscale_float();	/* scale a vector */
    void      splarax_float();	/* matrix vector multiply */
    void      update_float();	/* add a scalar multiple of a vector to another */
    void      sorthog_float();	/* orthogonalize a vector against a list of others */
    void      bail();		/* our exit routine */
    void      scanmin();	/* find small values in vector, store in linked list */
    void      frvec();		/* free vector */
    void      frvec_float();	/* free vector */
    void      scadd_float();	/* add scalar multiple of vector to another */
    void      scadd_mixed();	/* add scalar multiple of vector to another */
    void      orthog1_float();	/* efficiently orthogonalize against vector of ones */
    void      vecran_float();	/* fill vector with random entries */
    void      solistout_float();/* print out orthogonalization list */
    void      doubleout();	/* print a double precision number */
    void      orthogvec_float();/* orthogonalize one vector against another */
    void      double_to_float();/* copy a double vector to a float vector */
    void      float_to_double();/* convert float to double vector */
    void      warnings();       /* post various warnings about computation */
    void      strout();		/* print string to screen and output file */


    if (DEBUG_TRACE > 0) {
	printf("<Entering lanczos_so_float>\n");
    }

    if (DEBUG_EVECS > 0) {
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
    u = mkvec_float(1, n);
    u_double = mkvec(1, n);
    r = mkvec_float(1, n);
    workn = mkvec_float(1, n);
    workn_double = mkvec(1, n);
    Ares = mkvec(0, d);
    index = (int *) smalloc((unsigned) (d + 1) * sizeof(int));
    alpha = mkvec(1, maxj);
    beta = mkvec(0, maxj);
    ritz = mkvec(1, maxj);
    s = mkvec(1, maxj);
    bj = mkvec(1, maxj);
    workj = mkvec(0, maxj);
    q = (float **) smalloc((unsigned) (maxj + 1) * sizeof(float *));
    solist = (struct orthlink_float **) smalloc((unsigned) (maxj + 1) * sizeof(struct orthlink_float *));
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
	printf("  maxdeg %g\n", maxdeg);
	printf("  goodtol %g\n", goodtol);
	printf("  interval %d\n", interval);
	printf("  maxj %d\n", maxj);
	if (LANCZOS_CONVERGENCE_MODE == 1)
	    printf("  assigntol %d\n", assigntol);
    }
 
    /* Make a float copy of vwsqrt */
    if (vwsqrt == NULL) {
	vwsqrt_float = NULL;
    }
    else {
        vwsqrt_float = mkvec_float(0,n);
        double_to_float(vwsqrt_float,1,n,vwsqrt);
    }

    /* Initialize space. */
    vecran_float(r, 1, n);
    if (vwsqrt_float == NULL) {
	orthog1_float(r, 1, n);
    }
    else {
	orthogvec_float(r, 1, n, vwsqrt_float);
    }
    beta[0] = norm_float(r, 1, n);
    q[0] = mkvec_float(1, n);
    setvec_float(q[0], 1, n, 0.0);
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
	q[j] = mkvec_ret_float(1, n);
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
	        frvec_float(q[i], 1);
    	    }
            j = lastpause;
	}

	vecscale_float(q[j], 1, n, (float)(1.0 / beta[j - 1]), r);
	blas_time += lanc_seconds() - time;
	time = lanc_seconds();
	splarax_float(u, A, n, q[j], vwsqrt_float, workn);
	splarax_time += lanc_seconds() - time;
	time = lanc_seconds();
	update_float(r, 1, n, u, (float)(-beta[j - 1]), q[j - 1]);
	alpha[j] = dot_float(r, 1, n, q[j]);
	update_float(r, 1, n, r, (float)(-alpha[j]), q[j]);
	blas_time += lanc_seconds() - time;
	time = lanc_seconds();
	if (vwsqrt_float == NULL) {
	    orthog1_float(r, 1, n);
	}
	else {
	    orthogvec_float(r, 1, n, vwsqrt_float);
	}
	if ((j == (lastpause + 1)) || (j == (lastpause + 2))) {
	    sorthog_float(r, n, solist, ngood);
	}
	orthog_time += lanc_seconds() - time;
	beta[j] = norm_float(r, 1, n);
	time = lanc_seconds();
	pause = lanpause_float(j, lastpause, interval, q, n, &pausemode, version, beta[j]);
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
		    frvec_float(q[i], 1);
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
			i = d;
			curlnk = scanlist;
			while (curlnk != NULL) {
			    lambda[i] = curlnk->val;
			    bound[i] = bj[curlnk->indx];
			    index[i] = curlnk->indx;
			    curlnk = curlnk->pntr;
			    i--;
			}
			for (i = 1; i <= d; i++) {
			    Sres = Tevec(alpha, beta - 1, j, lambda[i], s);
			    if (Sres > Sres_max) {
				Sres_max = Sres;
			    }
			    if (Sres > SRESTOL) {
				inc_bis_safety = TRUE;
			    }
			    setvec(y[i], 1, n, 0.0);
			    for (k = 1; k <= j; k++) {
				scadd_mixed(y[i], 1, n, s[k], q[k]);
			    }
			}
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
		    i = d;
		    curlnk = scanlist;
		    while (curlnk != NULL) {
			lambda[i] = curlnk->val;
			bound[i] = bj[curlnk->indx];
			index[i] = curlnk->indx;
			curlnk = curlnk->pntr;
			i--;
		    }
		    for (i = 1; i <= d; i++) {
			Sres = Tevec(alpha, beta - 1, j, lambda[i], s);
			if (Sres > Sres_max) {
			    Sres_max = Sres;
			}
			if (Sres > SRESTOL) {
			    inc_bis_safety = TRUE;
			}
			setvec(y[i], 1, n, 0.0);
			for (k = 1; k <= j; k++) {
			    scadd_mixed(y[i], 1, n, s[k], q[k]);
			}
		    }

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
		/* Collect eigenvalue and bound information for display, return. */
		i = d;
		curlnk = scanlist;
		while (curlnk != NULL) {
		    lambda[i] = curlnk->val;
		    bound[i] = bj[curlnk->indx];
		    index[i] = curlnk->indx;
		    curlnk = curlnk->pntr;
		    i--;
		}

		/* Compute eigenvectors and display associated info. */
		printf("j %4d;    lambda                Ares est.             Ares          index\n",j);
		for (i = 1; i <= d; i++) {
		    Sres = Tevec(alpha, beta - 1, j, lambda[i], s);
		    if (Sres > Sres_max) {
			Sres_max = Sres;
		    }
		    if (Sres > SRESTOL) {
			inc_bis_safety = TRUE;
		    }
		    setvec(y[i], 1, n, 0.0);
		    for (k = 1; k <= j; k++) {
			scadd_mixed(y[i], 1, n, s[k], q[k]);
		    }
		    float_to_double(u_double,1,n,u);
		    Ares[i] = checkeig(workn_double, A, y[i], n, lambda[i], vwsqrt, u_double);
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
			    solist[ngood] = makeorthlnk_float();
			    (solist[ngood])->vec = mkvec_float(1, n);
			}
			(solist[ngood])->index = i;
			Sres = Tevec(alpha, beta - 1, j, ritz[i], s);
			if (Sres > Sres_max) {
			    Sres_max = Sres;
			}
			if (Sres > SRESTOL) {
			    inc_bis_safety = TRUE;
			}
			setvec_float((solist[ngood])->vec, 1, n, 0.0);
			for (k = 1; k <= j; k++) {
			    scadd_float((solist[ngood])->vec, 1, n, (float)(s[k]), q[k]);
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
			    solist[ngood] = makeorthlnk_float();
			    (solist[ngood])->vec = mkvec_float(1, n);
			}
			(solist[ngood])->index = i;
			Sres = Tevec(alpha, beta - 1, j, ritz[i], s);
			if (Sres > Sres_max) {
			    Sres_max = Sres;
			}
			if (Sres > SRESTOL) {
			    inc_bis_safety = TRUE;
			}
			setvec_float((solist[ngood])->vec, 1, n, 0.0);
			for (k = 1; k <= j; k++) {
			    scadd_float((solist[ngood])->vec, 1, n, (float)(s[k]), q[k]);
			}
		    }
		}
		ritz_time += lanc_seconds() - time;

		if (DEBUG_EVECS > 2) {
		    time = lanc_seconds();
		    printf("  j %3d; goodlim lft %2d, rgt %2d; list ",
			   j, left_goodlim, right_goodlim);
		    solistout_float(solist, n, ngood, j);
		    printf("---------------------end of iteration---------------------\n\n");
		    debug_time += lanc_seconds() - time;
		}
	    }
	}
	j++;
    }
    j--;

    /* Collect eigenvalue and bound information. Only compute and display info for
       the eigpairs actually used in the partitioning since don't want to spend the
       time or space to compute the null-space of the Laplacian. */
    time = lanc_seconds();
    i = d;
    curlnk = scanlist;
    while (curlnk != NULL) {
	lambda[i] = curlnk->val;
	bound[i] = bj[curlnk->indx];
	index[i] = curlnk->indx;
	curlnk = curlnk->pntr;
	i--;
    }
    scan_time += lanc_seconds() - time;

    /* Compute eigenvectors. */
    time = lanc_seconds();
    for (i = 1; i <= d; i++) {
	Sres = Tevec(alpha, beta - 1, j, lambda[i], s);
	if (Sres > Sres_max) {
	    Sres_max = Sres;
	}
	setvec(y[i], 1, n, 0.0);
	for (k = 1; k <= j; k++) {
	    scadd_mixed(y[i], 1, n, s[k], q[k]);
	}
    }
    evec_time += lanc_seconds() - time;

    time = lanc_seconds();
    float_to_double(u_double,1,n,u);
    warnings(workn_double, A, y, n, lambda, vwsqrt, Ares, bound, index,
             d, j, maxj, Sres_max, eigtol, u_double, Anorm, Output_File);
    debug_time += lanc_seconds() - time;

    /* free up memory */
    time = lanc_seconds();
    frvec_float(u, 1);
    frvec(u_double, 1);
    frvec_float(r, 1);
    frvec_float(workn, 1);
    frvec(workn_double, 1);
    frvec(Ares, 0);
    sfree((char *) index);
    frvec(alpha, 1);
    frvec(beta, 0);
    frvec(ritz, 1);
    frvec(s, 1);
    frvec(bj, 1);
    frvec(workj, 0);
    for (i = 0; i <= j; i++) {
	frvec_float(q[i], 1);
    }
    sfree((char *) q);
    if (vwsqrt_float != NULL) {
        frvec_float(vwsqrt_float,0);
    }
    while (scanlist != NULL) {
	curlnk = scanlist->pntr;
	sfree((char *) scanlist);
	scanlist = curlnk;
    }
    for (i = 1; i <= maxngood; i++) {
	frvec_float((solist[i])->vec, 1);
	sfree((char *) solist[i]);
    }
    sfree((char *) solist);
    if (LANCZOS_CONVERGENCE_MODE == 1) {
	sfree((char *) old_assignment);
    }
    init_time += lanc_seconds() - time;
}
