/* This software was developed by Bruce Hendrickson and Robert Leland   
   at Sandia National Laboratories under US Department of Energy  
   contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include <math.h>
#include "structs.h"
#include "defs.h"

/* See comments in lanczos_ext(). */

int       lanczos_ext_float(A, n, d, y, eigtol, vwsqrt, maxdeg, version, gvec, sigma)
struct vtx_data **A;		/* sparse matrix in row linked list format */
int       n;			/* problem size */
int       d;			/* problem dimension = number of eigvecs to find */
double  **y;			/* columns of y are eigenvectors of A  */
double    eigtol;		/* tolerance on eigenvectors */
double   *vwsqrt;		/* square roots of vertex weights */
double    maxdeg;		/* maximum degree of graph */
int       version;		/* flags which version of sel. orth. to use */
double    *gvec;		/* the rhs n-vector in the extended eigen problem */
double    sigma;		/* specifies the norm constraint on extended
				   eigenvector */
{
    extern FILE *Output_File;		/* output file or null */
    extern int LANCZOS_SO_INTERVAL;	/* interval between orthogonalizations */
    extern int LANCZOS_MAXITNS;         /* maximum Lanczos iterations allowed */
    extern int DEBUG_EVECS;		/* print debugging output? */
    extern int DEBUG_TRACE;		/* trace main execution path */
    extern int WARNING_EVECS;		/* print warning messages? */
    extern double BISECTION_SAFETY;	/* safety factor for T bisection */
    extern double SRESTOL;		/* resid tol for T evec comp */
    extern double DOUBLE_EPSILON;	/* machine precision */
    extern double DOUBLE_MAX;		/* largest double value */
    extern double splarax_time; /* time matvec */
    extern double orthog_time;  /* time orthogonalization work */
    extern double evec_time;    /* time to generate eigenvectors */
    extern double ql_time;      /* time tridiagonal eigenvalue work */
    extern double blas_time;    /* time for blas. linear algebra */
    extern double init_time;    /* time to allocate, intialize variables */
    extern double scan_time;    /* time for scanning eval and bound lists */
    extern double debug_time;   /* time for (some of) debug computations */
    extern double ritz_time;    /* time to generate ritz vectors */
    extern double pause_time;   /* time to compute whether to pause */
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
    double    bis_safety;	/* real safety factor for T bisection */
    double    Sres;		/* how well Tevec calculated eigvec s */
    double    Sres_max;		/* Max value of Sres */
    int       inc_bis_safety;	/* need to increase bisection safety */
    double   *Ares;		/* how well Lanczos calc. eigpair lambda,y */
    int      *index;		/* the Ritz index of an eigenpair */
    struct orthlink_float **solist;	/* vec. of structs with vecs. to orthog. against */
    struct scanlink *scanlist;		/* linked list of fields to do with min ritz vals */
    struct scanlink *curlnk;		/* for traversing the scanlist */
    double    bji_tol;		/* tol on bji est. of eigen residual of A */
    int       converged;	/* has the iteration converged? */
    double    goodtol;		/* error tolerance for a good Ritz vector */
    int       ngood;		/* total number of good Ritz pairs at current step */
    int       maxngood;		/* biggest val of ngood through current step */
    int       left_ngood;	/* number of good Ritz pairs on left end */
    int       lastpause;	/* Most recent step with good ritz vecs */
    int       nopauses;		/* Have there been any pauses? */
    int       interval;		/* number of steps between pauses */
    double    time;             /* Current clock time */
    int       left_goodlim;	/* number of ritz pairs checked on left end */
    double    Anorm;		/* Norm estimate of the Laplacian matrix */
    int       pausemode;	/* which Lanczos pausing criterion to use */
    int       pause;		/* whether to pause */
    int       temp;		/* used to prevent redundant index computations */
    double   *extvec;		/* n-vector solving the extended A eigenproblem */
    double   *v;		/* j-vector solving the extended T eigenproblem */
    double    extval;		/* computed extended eigenvalue (of both A and T) */
    double   *work1, *work2;    /* work vectors */
    double    check;		/* to check an orthogonality condition */
    double    numerical_zero;	/* used for zero in presense of round-off  */
    int       ritzval_flag;	/* status flag for get_ritzvals() */
    double    resid;		/* residual */
    int       memory_ok;	/* TRUE until memory runs out */
    float    *vwsqrt_float;     /* float version of vwsqrt */

    struct orthlink_float *makeorthlnk_float();	/* makes space for new entry in orthog. set */
    struct scanlink *mkscanlist();		/* init scan list for min ritz vecs */
    double   *mkvec();			/* allocates space for a vector */
    float    *mkvec_float();		/* allocates space for a vector */
    float    *mkvec_ret_float();	/* mkvec() which returns error code */
    double   *smalloc();		/* safe version of malloc */
    double    dot_float();		/* standard dot product routine */
    double    norm();			/* vector norm */
    double    norm_float();		/* vector norm */
    double    Tevec();			/* calc eigenvector of T by linear recurrence */
    double    lanc_seconds();   	/* switcheable timer */
    int       sfree();          	/* free allocated memory safely */
    int       lanpause_float();      	/* figure when to pause Lanczos iteration */
    int       get_ritzvals();   	/* compute eigenvalues of T */
    void      setvec();         	/* initialize a vector */
    void      setvec_float();         	/* initialize a vector */
    void      vecscale_float();     	/* scale a vector */
    void      splarax();        	/* matrix vector multiply */
    void      splarax_float();        	/* matrix vector multiply */
    void      update_float();         	/* add scalar multiple of a vector to another */
    void      sorthog_float();        	/* orthogonalize vector against list of others */
    void      bail();           	/* our exit routine */
    void      scanmin();        	/* store small values of vector in linked list */
    void      frvec();         		/* free vector */
    void      frvec_float();          	/* free vector */
    void      scadd();          	/* add scalar multiple of vector to another */
    void      scadd_float();          	/* add scalar multiple of vector to another */
    void      scadd_mixed();          	/* add scalar multiple of vector to another */
    void      orthog1_float();        	/* efficiently orthog. against vector of ones */
    void      solistout_float();      	/* print out orthogonalization list */
    void      doubleout();      	/* print a double precision number */
    void      orthogvec_float();      	/* orthogonalize one vector against another */
    void      double_to_float();	/* copy a double vector to a float vector */
    void      get_extval();		/* find extended Ritz values */
    void      scale_diag();		/* scale vector by diagonal matrix */
    void      scale_diag_float();	/* scale vector by diagonal matrix */
    void      strout();			/* print string to screen and file */

    if (DEBUG_TRACE > 0) {
	printf("<Entering lanczos_ext_float>\n");
    }

    if (DEBUG_EVECS > 0) {
	printf("Selective orthogonalization Lanczos for extended eigenproblem, matrix size = %d.\n", n);
    }

    /* Initialize time. */
    time = lanc_seconds();

    if (d != 1) {
	bail("ERROR: Extended Lanczos only available for bisection.",1);
        /* ... something must be wrong upstream. */
    }

    if (n < d + 1) {
	bail("ERROR: System too small for number of eigenvalues requested.",1);
	/* ... d+1 since don't use zero eigenvalue pair */
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
    extvec = mkvec(1, n);
    v = mkvec(1, maxj);
    work1 = mkvec(1, maxj);
    work2 = mkvec(1, maxj);

    /* Set some constants governing orthogonalization */
    ngood = 0;
    maxngood = 0;
    bji_tol = eigtol;
    Anorm = 2 * maxdeg;				/* Gershgorin estimate for ||A|| */
    goodtol = Anorm * sqrt(DOUBLE_EPSILON);	/* Parlett & Scott's bound, p.224 */
    interval = 2 + (int) min(LANCZOS_SO_INTERVAL - 2, n / (2 * LANCZOS_SO_INTERVAL));
    bis_safety = BISECTION_SAFETY;
    numerical_zero = 1.0e-6;

    if (DEBUG_EVECS > 0) {
	printf("  maxdeg %g\n", maxdeg);
	printf("  goodtol %g\n", goodtol);
	printf("  interval %d\n", interval);
        printf("  maxj %d\n", maxj);
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
    double_to_float(r,1,n,gvec);
    if (vwsqrt_float != NULL) {
	scale_diag_float(r,1,n,vwsqrt_float);
    }
    check = norm_float(r,1,n);
    if (vwsqrt_float == NULL) {
	orthog1_float(r, 1, n);
    }
    else { 
	orthogvec_float(r, 1, n, vwsqrt_float);
    }
    check = fabs(check - norm_float(r,1,n));
    if (check > 10*numerical_zero && WARNING_EVECS > 0) {
	strout("WARNING: In terminal propagation, rhs should have no component in the"); 
        printf("         nullspace of the Laplacian, so check val %g should be zero.\n", check); 
	if (Output_File != NULL) {
            fprintf(Output_File,
		"         nullspace of the Laplacian, so check val %g should be zero.\n",
	    check); 
	}
    }
    beta[0] = norm_float(r, 1, n);
    q[0] = mkvec_float(1, n);
    setvec_float(q[0], 1, n, 0.0);
    setvec(bj, 1, maxj, DOUBLE_MAX);

    if (beta[0] < numerical_zero) {
     /* The rhs vector, Dg, of the transformed problem is numerically zero or is
	in the null space of the Laplacian, so this is not a well posed extended
	eigenproblem. Set maxj to zero to force a quick exit but still clean-up
	memory and return(1) to indicate to eigensolve that it should call the
	default eigensolver routine for the standard eigenproblem. */
	maxj = 0;
    }
	
    /* Main Lanczos loop. */
    j = 1;
    lastpause = 0;
    pausemode = 1;
    left_ngood = 0;
    left_goodlim = 0;
    converged = FALSE;
    Sres_max = 0.0;
    inc_bis_safety = FALSE;
    nopauses = TRUE;
    memory_ok = TRUE;
    init_time += lanc_seconds() - time;
    while ((j <= maxj) && (!converged) && memory_ok) {
        time = lanc_seconds();

	/* Allocate next Lanczos vector. If fail, back up to last pause. */
	q[j] = mkvec_ret_float(1, n);
        if (q[j] == NULL) {
	    memory_ok = FALSE;
  	    if (DEBUG_EVECS > 0 || WARNING_EVECS > 0) {
                strout("WARNING: Lanczos_ext out of memory; computing best approximation available.\n");
            }
	    if (nopauses) {
	        bail("ERROR: Sorry, can't salvage Lanczos_ext.",1); 
  	        /* ... save yourselves, men.  */
	    }
    	    for (i = lastpause+1; i <= j-1; i++) {
	        frvec_float(q[i], 1);
    	    }
            j = lastpause;
	}

        /* Basic Lanczos iteration */
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

        /* Selective orthogonalization */
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
	    lastpause = j;

	    /* Compute limits for checking Ritz pair convergence. */
	    if (version == 2) {
		if (left_ngood + 2 > left_goodlim) {
		    left_goodlim = left_ngood + 2;
		}
	    }

	    /* Special case: need at least d Ritz vals on left. */
	    left_goodlim = max(left_goodlim, d);

	    /* Special case: can't find more than j total Ritz vals. */
	    if (left_goodlim > j) {
		left_goodlim = min(left_goodlim, j);
	    }

	    /* Find Ritz vals using faster of Sturm bisection or ql. */
            time = lanc_seconds();
	    if (inc_bis_safety) {
		bis_safety *= 10;
		inc_bis_safety = FALSE;
	    }
	    ritzval_flag = get_ritzvals(alpha, beta, j, Anorm, workj, ritz, d,
			 left_goodlim, 0, eigtol, bis_safety);
            ql_time += lanc_seconds() - time;

	    if (ritzval_flag != 0) {
                bail("ERROR: Lanczos_ext failed in computing eigenvalues of T.",1);
		/* ... we recover from this in lanczos_SO, but don't worry here. */ 
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
            ritz_time += lanc_seconds() - time;

	    /* Show the portion of the spectrum checked for convergence. */
	    if (DEBUG_EVECS > 2) {
                time = lanc_seconds();
		printf("\nindex         Ritz vals            bji bounds\n");
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
		    if ((temp > left_goodlim) && (temp < j)) {
			printf("  %3d", temp);
			doubleout(ritz[temp], 1);
			doubleout(bj[temp], 1);
			printf("\n");
		    }
		    curlnk = curlnk->pntr;
		}
		printf("                            -------------------\n");
		printf("                goodtol:    %19.16f\n\n", goodtol);
                debug_time += lanc_seconds() - time;
	    }

	    get_extval(alpha, beta, j, ritz[1], s, eigtol, beta[0], sigma, &extval,
		v, work1, work2);

	    /* check convergence of Ritz pairs */
            time = lanc_seconds();
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
            scan_time += lanc_seconds() - time;

	    if (!converged) {
		ngood = 0;
		left_ngood = 0;	/* for setting left_goodlim on next loop */

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
			    scadd_float((solist[ngood])->vec, 1, n, s[k], q[k]);
			}
		    }
		}
                ritz_time += lanc_seconds() - time;

		if (DEBUG_EVECS > 2) {
                    time = lanc_seconds();
		    printf("  j %3d; goodlim lft %2d, rgt %2d; list ",
			   j, left_goodlim, 0);
		    solistout_float(solist, n, ngood, j);
                    printf("---------------------end of iteration---------------------\n\n");
                    debug_time += lanc_seconds() - time;
		}
	    }
	}
	j++;
    }
    j--;

    if (DEBUG_EVECS > 0) {
        time = lanc_seconds();
	if (maxj == 0) {
	    printf("Not extended eigenproblem -- calling ordinary eigensolver.\n");
	}
	else {
	    printf("  Lanczos_ext itns: %d\n",j);
	    printf("  eigenvalue: %g\n",ritz[1]);
	    printf("  extended eigenvalue: %g\n",extval);
	}
       debug_time += lanc_seconds() - time;
    }

    if (maxj != 0) {
        /* Compute (scaled) extended eigenvector. */
        time = lanc_seconds(); 
        setvec(y[1], 1, n, 0.0);
        for (k = 1; k <= j; k++) {
            scadd_mixed(y[1], 1, n, v[k], q[k]);
        }
        evec_time += lanc_seconds() - time;
        /* Note: assign() will scale this y vector back to x (since y = Dx) */ 

       /* Compute and check residual directly. Use the Ay = extval*y + Dg version of
          the problem for convenience. Note that u and v are used here as workspace */ 
        time = lanc_seconds(); 
        splarax(workn_double, A, n, y[1], vwsqrt, u_double);
        scadd(workn_double, 1, n, -extval, y[1]);
        scale_diag(gvec,1,n,vwsqrt);
        scadd(workn_double, 1, n, -1.0, gvec);
        resid = norm(workn_double, 1, n);
        if (DEBUG_EVECS > 0) {
	    printf("  extended residual: %g\n",resid);
	    if (Output_File != NULL) {
	        fprintf(Output_File, "  extended residual: %g\n",resid);
	    }
	}
	if (WARNING_EVECS > 0 && resid > eigtol) {
	    printf("WARNING: Extended residual (%g) greater than tolerance (%g).\n",
		resid, eigtol);
	    if (Output_File != NULL) {
                fprintf(Output_File,
		    "WARNING: Extended residual (%g) greater than tolerance (%g).\n",
		    resid, eigtol);
	    }
	}
        debug_time += lanc_seconds() - time;
    } 


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
    frvec(extvec, 1);
    frvec(v, 1);
    frvec(work1, 1);
    frvec(work2, 1);
    init_time += lanc_seconds() - time;

    if (maxj == 0) return(1);  /* see note on beta[0] and maxj above */
    else return(0);
}
