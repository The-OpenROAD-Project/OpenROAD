/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <math.h>
#include <stdio.h>

/* Finds first extended eigenpair of system corresponding to
   tridiagonal T using using Rafael's bisection technique. */

void      get_extval(alpha, beta, j, ritzval, s, eigtol, wnorm_g, sigma, extval, v, work1, work2)
double   *alpha;	/* j-vector of Lanczos scalars (using elements 1 to j) */
double   *beta;		/* (j+1)-vector of " " (has 0 element but using 1 to j-1) */
int       j;		/* number of Lanczos iterations taken */
double    ritzval;	/* Ritz value */
double   *s;		/* Ritz vector (length n, re-computed in this routine) */
double    eigtol;	/* tolerance on eigenpair */
double    wnorm_g;	/* W-norm of n-vector g, the rhs in the extended eig. problem */
double    sigma;	/* the norm constraint on the extended eigenvector */
double   *extval;	/* the extended eigenvalue this routine computes */
double   *v;		/* the j-vector solving the extended eig problem in T */
double   *work1;	/* j-vector of workspace */
double   *work2;	/* j-vector of workspace */
{
    extern int DEBUG_EVECS;	/* debug flag for eigen computation */
    double    lambda_low;	/* lower bound on extended eval */
    double    lambda_high;	/* upper bound on extended eval */
    double    tol;		/* bisection tolerance */
    double    norm_v;		/* norm of the extended T eigenvector v */
    double    lambda;		/* the parameter that iterates to extval */
    int       cnt;		/* debug iteration counter */
    double    diff;		/* distance between lambda limits */
    double    norm(), Tevec();
    void      tri_solve(), cpvec();

    /* Compute the Ritz vector */
    Tevec(alpha, beta - 1, j, ritzval, s);

    /* Shouldn't happen, but just in case ... */
    if (wnorm_g == 0.0) {
	*extval = ritzval;
	cpvec(v, 1, j, s);
	if (DEBUG_EVECS > 0) {
	    printf("Degenerate extended eigenvector problem (g = 0).\n");
	}
	return;
	/* ... not really an extended eigenproblem; just return Ritz pair */
    }

    /* Set up the bisection parameters */
    lambda_low = ritzval - wnorm_g / sigma;
    lambda_high = ritzval - (wnorm_g / sigma) * s[1];
    lambda = 0.5 * (lambda_low + lambda_high);
    tol = eigtol * eigtol * (1 + fabs(lambda_low) + fabs(lambda_high));

    if (DEBUG_EVECS > 2) {
	printf("Computing extended eigenpairs of T\n");
	printf("  target norm_v (= sigma) %g\n", sigma);
	printf("  bisection tolerance %g\n", tol);
    }
    if (DEBUG_EVECS > 3) {
	printf("  lambda iterates to the extended eigenvalue\n");
	printf("         lambda_low           lambda            lambda_high      norm_v\n");
    }

    /* Bisection loop - iterate until norm constraint is satisfied */
    cnt = 1;
    diff = 2*tol;
    while (diff > tol) {
	lambda = 0.5 * (lambda_low + lambda_high);
	tri_solve(alpha, beta, j, lambda, v, wnorm_g, work1, work2);
	norm_v = norm(v, 1, j);
	if (DEBUG_EVECS > 3) {
	    printf("%2i   %18.16f  %18.16f  %18.16f  %g\n",
		   cnt++, lambda_low, lambda, lambda_high, norm_v);
	}
	if (norm_v <= sigma)
	    lambda_low = lambda;
	if (norm_v >= sigma)
	    lambda_high = lambda;
        diff = lambda_high - lambda_low;
    }

    /* Return the extended eigenvalue (eigvec is automatically returned) */
    *extval = lambda;
}
