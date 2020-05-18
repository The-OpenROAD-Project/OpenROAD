/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include <math.h>
#include "defs.h"
#include "params.h"

int SRES_SWITCHES = 0;	/* # switchs to backup routine for computing evec of T */

/* NOTE: Uses a modified form of the bidirectional recurrence similar to Parlett 
         and Reid's version. Made a minor correction and modified one heuristic
         that didn't seem to work well for our class of graphs. Code switches to 
	 Tinvit, Eispack's inverse iteration routine if the bidirectional recurrence 
	 fails to meet tolerance. Switches back to the bidirectional recurrence result 
	 if Tinvit fails to converge or gives a worse residual. There's code for the 
	 simple forward and backward recurrences in the comments at the end. */ 

/* NOTE: In this routine the diagonal of T is alpha[1] ... alpha[j], and the
         off-diagonal is beta[2] ... beta[j]. Because the various literature
         sources do not agree on indexing, some of the Lanczos algorithms have 
         to call this routine with the beta vector off-set by one. */

/* NOTE: The residual calculations look too simple, but they are right because
         the recurrences force (in exact arithmatic) most of the terms in the
         residual (T - ritz*I)s to zero. It's only the end points in the forward
         or backward recurrences or the merge point in the bidirectional recurrence
         where the resdual is not forced to zero. The values here are the residuals
         if we ignore round-off in the other terms (which appears to be valid). */

/* NOTE: This routine is expected to return a normalized vector s such that it's
         last entry, s[j], is positive, hence the calls to sign_normalize(). This
         is not an issue in the backward recurrence since we set s[j] positive. */

/* Finds eigenvector s of T and returns residual norm. */
double    Tevec(alpha, beta, j, ritz, s)
double   *alpha;		/* vector of Lanczos scalars */
double   *beta;			/* vector of Lanczos scalars */
int       j;			/* number of Lanczos iterations taken */
double    ritz;			/* approximate eigenvalue  of T */
double   *s;			/* approximate eigenvector of T */
{
    extern double SRESTOL;      /* limit on relative residual tol for evec of T */
    extern double DOUBLE_MAX;	/* maximum double precision value */
    int       i;		/* index */
    double    residual;		/* how well recurrence gives eigenvector */
    double    temp;		/* used to compute residual */
    double   *work;		/* temporary work vector allocated within if used */
    double    w[MAXDIMS + 1];	/* holds eigenvalue for tinvit */
    long      index[MAXDIMS +1];/* index vector for tinvit */
    long      ierr;		/* error flag for tinvit */
    long      nevals;		/* number of evals sought */
    long      long_j;		/* long copy of j for tinvit interface */
    double    hurdle;		/* hurdle for local maximum in recurrence */
    double    prev_resid;	/* stores residual from previous computation */

    int       tinvit_();	/* eispack's tinvit for evecs of symmetric T */
    double   *mkvec();		/* allocates double vectors */
    void      frvec();		/* frees double vectors */
    double    bidir();		/* bidirectional recurrence for evec of T */
    void      cpvec();		/* vector copy routine */

	s[1] = 1.0;

	if (j == 1) {
	    residual = fabs(alpha[1] - ritz);
	}

	if (j >= 2) { 
	    /*Bidirectional recurrence - corrected and modified from Parlett and Reid, 
              "Tracking the Progress of the Lanczos Algorithm ..., IMA JNA 1, 1981 */
	    hurdle = 1.0;
	    residual = bidir(alpha,beta,j,ritz,s,hurdle);
	}

	if (residual > SRESTOL) {
	    /* Try again with Eispack's Tinvit iteration */
	    SRES_SWITCHES++;
	    index[1] = 1;
	    work = mkvec(1, 7*j);	/* lump things to save mallocs */
	    w[1] = ritz;
	    work[1] = 0;
	    for (i = 2; i <= j; i++) {
	        work[i] = beta[i] * beta[i];
	    }
	    nevals = 1;
	    long_j = j;

	    /* save the previously computed evec in case it's better */ 
	    cpvec(&(work[6*j]),1,j,s);
	    prev_resid = residual;

	    tinvit_(&long_j, &long_j, &(alpha[1]), &(beta[1]), &(work[1]), &nevals,
		&(w[1]), &(index[1]), &(s[1]), &ierr, &(work[j+1]), &(work[(2*j)+1]), 
	    	&(work[(3*j)+1]), &(work[(4*j)+1]), &(work[(5*j)+1]));

	    /* fix up sign if needed */
	    if (s[j] < 0) {
	        for(i=1; i<=j; i++) {
		    s[i] = - s[i];
	        }
	    }

	    if (ierr != 0) {
	        residual = DOUBLE_MAX;
	        /* ... don't want to use evec since it is set to zero */
	    }

	    else {
	        temp = (alpha[1] - ritz) * s[1] + beta[2] * s[2];
	        residual = temp * temp;
	        for (i = 2; i < j; i++) {
		    temp = beta[i] * s[i - 1] + (alpha[i] - ritz) * s[i] 
                         + beta[i + 1] * s[i + 1];
		    residual += temp * temp;
	        }
	        temp = beta[j] * s[j - 1] + (alpha[j] - ritz) * s[j];
	        residual += temp * temp;
	        residual = sqrt(residual);
	        /* tinvit normalizes, so we don't need to. */
	    }

	    /* restore previous evec if it had a better residual */
	    if (prev_resid < residual) {
	        residual = prev_resid;
	        cpvec(s,1,j,&(work[6*j]));
	    	SRES_SWITCHES++; /* count since switching back as well */
	    }

	    frvec(work, 1);
	}

    return (residual);
}


	/* Keep this code around in case we have problems with the 
	   bidirectional recurrence. */

/* 	Backward recurrence 
	s[j] = 1.0;

	if (j == 1) {
	    residual = (alpha[1] - ritz);
	}

	if (j >= 2) {
	    s[j - 1] = -(alpha[j] - ritz) / beta[j];
	    for (i = j; i >= 3; i--) {
		s[i-2] = -((alpha[i - 1] - ritz) * s[i - 1] 
				+ beta[i] * s[i]) / beta[i - 1];
	    }
	    residual = (alpha[1] - ritz) * s[1] + beta[2] * s[2];
	}
	residual = fabs(residual) / normalize(s,1,j); 
*/


/* 	Forward recurrence
	s[1] = 1.0;

	if (j == 1) {
	    residual = (alpha[1] - ritz);
	}

	if (j >= 2) {
	    s[2] = -(alpha[1] - ritz) / beta[2];
	    for (i = 3; i <= j; i++) {
		s[i] = -((alpha[i - 1] - ritz) * s[i - 1] 
				+ beta[i - 1] * s[i - 2]) / beta[i];
	    }
	    residual = (alpha[j] - ritz) * s[j] + beta[j] * s[j - 1];
	}
 	residual = fabs(residual) / sign_normalize(s,1,j,j);
*/
