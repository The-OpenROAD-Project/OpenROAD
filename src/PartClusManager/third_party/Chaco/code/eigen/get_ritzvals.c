/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <math.h>
#include <stdio.h>
#include "defs.h"

/* Finds needed eigenvalues of tridiagonal T using either the QL algorithm 
   or Sturm sequence bisection, whichever is predicted to be faster based 
   on a simple complexity model. If one fails (which is rare), the other 
   is tried. The return value is 0 if one of the routines succeeds. If they
   both fail, the return value is 1, and Lanczos should compute the best
   approximation it can based on previous iterations. */ 
int       get_ritzvals(alpha, beta, j, Anorm, workj, ritz, d, left_goodlim,
		       right_goodlim, eigtol, bis_safety)
double   *alpha;		/* vector of Lanczos scalars */
double   *beta;			/* vector of Lanczos scalars */
int       j;			/* number of Lanczos iterations taken */
double    Anorm;		/* Gershgorin estimate */
double   *workj;		/* work vector for Sturm sequence */
double   *ritz;			/* array holding evals */
int       d;			/* problem dimension = num. eigenpairs needed */
int       left_goodlim;		/* number of ritz pairs checked on left end */
int       right_goodlim;	/* number of ritz pairs checked on right end */
double    eigtol;		/* tolerance on eigenpair */
double    bis_safety;		/* bisection tolerance function divisor */
{
    extern int DEBUG_EVECS;	/* debug flag for eigen computation */
    extern int WARNING_EVECS;	/* warning flag for eigen computation */
    int       nvals_left;	/* numb. evals to find on left end of spectrum */
    int       nvals_right;	/* numb. evals to find on right end of spectrum */
    double    bisection_tol;	/* width of interval bisection should converge to */
    int       pred_steps;	/* predicts # of required bisection steps per eval */
    int       tot_pred_steps;	/* predicts total # of required bisection steps */
    double   *ritz_sav = NULL;	/* copy of ritzvals for debugging */
    int       bisect_flag;	/* return status of bisect() */
    int       ql_flag;		/* return status of ql() */
    int       local_debug;	/* whether to check bisection results with ql */ 
    int       bisect();		/* locates eigvals using bisection on Sturm seq. */
    int       ql(); 		/* computes eigenvalues of T using eispack algorithm */
    void      shell_sort(); 	/* sorts vector of eigenvalues */
    double   *mkvec();		/* to allocate a vector */
    void      frvec();		/* free vector */
    void      cpvec();		/* vector copy */
    void      bail();		/* our exit routine */
    void      strout();		/* string out to screen and output file */

    /* Determine number of ritzvals to find on left and right ends */
    nvals_left = max(d, left_goodlim);
    nvals_right = min(j - nvals_left, right_goodlim);

    /* Estimate work for bisection vs. ql assuming bisection takes 5j flops per
       step, ql takes 30j^2 flops per call. (Ignore sorts, copies, addressing.) */

    bisection_tol = eigtol * eigtol / bis_safety;
    pred_steps = (log10(Anorm / bisection_tol) / log10(2.0)) + 1; 
    tot_pred_steps = (nvals_left + nvals_right) * pred_steps;

    bisect_flag = ql_flag = 0;

    if (5 * tot_pred_steps < 30 * j) { 
	if (DEBUG_EVECS > 2) printf("  tridiagonal solver: bisection\n");

        /* Set local_debug = TRUE for a table checking bisection against QL. */
        local_debug = FALSE; 
        if (local_debug) {
    	    ritz_sav = mkvec(1,j);
	    cpvec(ritz_sav, 1, j, alpha);
	    cpvec(workj, 0, j, beta);
	    ql_flag = ql(ritz_sav, workj, j);
	    if (ql_flag != 0) {
	        bail("Aborting debugging procedure in get_ritzvals().\n",1);
	    } 
	    shell_sort(j, ritz_sav);
        }

	bisect_flag = bisect(alpha, beta, j, Anorm, workj, ritz, nvals_left, 
            nvals_right, bisection_tol, ritz_sav, pred_steps + 10);

        if (local_debug) frvec(ritz_sav,1); 
    }

    else {
	if (DEBUG_EVECS > 2) printf("  tridiagonal solver: ql\n");
        cpvec(ritz, 1, j, alpha);
        cpvec(workj, 0, j, beta);
	ql_flag = ql(ritz, workj, j);
	shell_sort(j, ritz);
    }

    if (bisect_flag != 0 && ql_flag == 0) {
	if (DEBUG_EVECS > 0 || WARNING_EVECS > 0) {
	    strout("WARNING: Sturm bisection of T failed; switching to QL.\n");
        }
	if (DEBUG_EVECS > 1 || WARNING_EVECS > 1) {
	    if (bisect_flag == 1) 
	        strout("         - failure detected in sturmcnt().\n");
	    if (bisect_flag == 2) 
	        strout("         - maximum number of bisection steps reached.\n");
        }
        cpvec(ritz, 1, j, alpha);
        cpvec(workj, 0, j, beta);
	ql_flag = ql(ritz, workj, j);
	shell_sort(j, ritz);
    } 

    if (ql_flag != 0 && bisect_flag == 0) {
	if (DEBUG_EVECS > 0 || WARNING_EVECS > 0) {
	    strout("WARNING: QL failed for T; switching to Sturm bisection.\n");
        }
	bisect_flag = bisect(alpha, beta, j, Anorm, workj, ritz, nvals_left, 
            nvals_right, bisection_tol, ritz_sav, pred_steps + 3);
    } 

    if (bisect_flag != 0 && ql_flag != 0) {
	if (DEBUG_EVECS > 0 || WARNING_EVECS > 0) {
	    return(1);		/* can't recover; bail out with error code */ 
  	}
    }

    return(0);   /* ... things seem ok. */
}
