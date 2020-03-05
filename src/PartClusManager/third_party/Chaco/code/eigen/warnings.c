/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include "defs.h"
#include "structs.h"

/* Post various warnings about computation  */
void      warnings(workn, A, y, n, lambda, vwsqrt, Ares, bound,
		           index, d, j, maxj, Sres_max, eigtol, u, Anorm, out_file)
double   *workn;		/* work vector (1..n) */
struct vtx_data **A;		/* graph */
double  **y;			/* eigenvectors */
int       n;			/* number of vtxs */
double   *lambda;		/* ritz approximation to eigenvals of A */
double   *vwsqrt;		/* square roots of vertex weights */
double   *Ares;			/* how well Lanczos calc. eigpair lambda,y */
int      *index;		/* the Ritz index of an eigenpair */
double   *bound;		/* on ritz pair approximations to eig pairs of A */
int       d;			/* problem dimension = number of eigvecs to find */
int       j;			/* number of Lanczos iterations used */
int       maxj;			/* maximum number of Lanczos iterations */
double    Sres_max;		/* Max value of Sres */
double    eigtol;		/* tolerance on eigenvectors */
double   *u;			/* Lanczos vector; here used as workspace */
double    Anorm;		/* Gershgorin bound on eigenvalue */
FILE     *out_file;		/* output file */
{
    extern int DEBUG_EVECS;	/* print debugging output? */
    extern int WARNING_EVECS;	/* print warning messages? */
    extern double WARNING_ORTHTOL;	/* Warning: modest loss of orthogonality */
    extern double WARNING_MISTOL;	/* Warning: serious loss of orthogonality */
    extern double SRESTOL;	/* limit on relative residual tol for evec of T */
    extern int LANCZOS_CONVERGENCE_MODE;	/* type of Lanczos convergence test */
    extern int SRES_SWITCHES;	/* # switches to backup routine for computing s */
    int       warning1;		/* warning1 cond. (eigtol not achieved) true? */
    int       warning2;		/* warning2 cond. (premature orth. loss) true? */
    int       warning3;		/* warning3 cond. (suspected misconvergence) true? */
    int       i;		/* loop index */
    int       hosed;		/* flag for serious Lanczos problems */
    int       pass;		/* which time through we are on */
    FILE     *outfile;		/* set to output file or stdout */
    double    checkeig();	/* calculate residual of eigenvector of A */
    void      doubleout_file();	/* print a double precision number */
    void      bail();		/* our exit routine */

    hosed = FALSE;
    for (pass = 1; pass <= 2; pass++) {

	if (pass == 1) {
	    outfile = stdout;
	}
	if (pass == 2) {
	    if (out_file != NULL) {
		outfile = out_file;
	    }
	    else if (hosed) {
		bail((char *) NULL, 1);
	    }
	    else {
		return;
	    }
	}

	if (DEBUG_EVECS > 0 || WARNING_EVECS > 0) {
	    if (LANCZOS_CONVERGENCE_MODE == 1) {
		fprintf(outfile, "Note about warnings: in partition convergence monitoring mode.\n");
	    }
	    for (i = 1; i <= d; i++) {
		Ares[i] = checkeig(workn, A, y[i], n, lambda[i], vwsqrt, u);
	    }
	}

	if (DEBUG_EVECS > 0) {
	    if (pass == 1) {
		fprintf(outfile, "Lanczos itns. = %d\n", j);
	    }
	    fprintf(outfile, "          lambda                Ares est.              Ares          index\n");
	    for (i = 1; i <= d; i++) {
		fprintf(outfile, "%2d.", i);
		doubleout_file(outfile, lambda[i], 1);
		doubleout_file(outfile, bound[i], 1);
		doubleout_file(outfile, Ares[i], 1);
		fprintf(outfile, "   %3d\n", index[i]);
	    }
	    fprintf(outfile, "\n");
	}

	if (WARNING_EVECS > 0) {
	    warning1 = FALSE;
	    warning2 = FALSE;
	    warning3 = FALSE;
	    for (i = 1; i <= d; i++) {
		if (Ares[i] > eigtol) {
		    warning1 = TRUE;
		}
		if (Ares[i] > WARNING_ORTHTOL * bound[i] && Ares[i] > .01 * eigtol) {
		    warning2 = TRUE;
		}
		if (Ares[i] > WARNING_MISTOL * bound[i] && Ares[i] > .01 * eigtol) {
		    warning3 = TRUE;
		}
	    }
	    if (j == maxj) {
		fprintf(outfile, "WARNING: Maximum number of Lanczos iterations reached.\n");
	    }
	    if (warning2 && !warning3) {
		fprintf(outfile, "WARNING: Minor loss of orthogonality (Ares/est. > %g).\n",
			WARNING_ORTHTOL);
	    }
	    if (warning3) {
		fprintf(outfile,
		  "WARNING: Substantial loss of orthogonality (Ares/est. > %g).\n",
			WARNING_MISTOL);
	    }
	    if (warning1) {
		fprintf(outfile, "WARNING: Eigen pair tolerance (%g) not achieved.\n", eigtol);
	    }
	}

	if (WARNING_EVECS > 1) {
	    if (warning1 || warning2 || warning3) {
		if (DEBUG_EVECS <= 0) {
		    fprintf(outfile,
			    "          lambda                Ares est.              Ares          index\n");
		    for (i = 1; i <= d; i++) {
			fprintf(outfile, "%2d.", i);
			doubleout_file(outfile, lambda[i], 1);
			doubleout_file(outfile, bound[i], 1);
			doubleout_file(outfile, Ares[i], 1);
			fprintf(outfile, "   %3d\n", index[i]);
		    }
		}
		/* otherwise gets printed above */
	    }
	}

	if (warning1 || warning2 || warning3 || WARNING_EVECS > 2) {
	    if (Sres_max > SRESTOL) {
		fprintf(outfile,
			"WARNING: Maximum eigen residual of T (%g) exceeds SRESTOL.\n", Sres_max);
	    }
	}

	if (WARNING_EVECS > 2) {
	    if (SRES_SWITCHES > 0) {
		fprintf(outfile, "WARNING: Switched routine for computing evec of T %d times.\n",
			SRES_SWITCHES);
		SRES_SWITCHES = 0;
	    }
	}

	/* Put the best face on things ... */
	for (i = 1; i <= d; i++) {
	    if (lambda[i] < 0 || lambda[i] > Anorm + eigtol) {
		hosed = TRUE;
	    }
	}
	if (hosed) {
	    fprintf(outfile,
		    "ERROR: Sorry, out-of-bounds eigenvalue indicates serious breakdown.\n");
	    fprintf(outfile,
		    "       Try different parameters or another eigensolver.\n");
	    if (pass == 2)
		bail((char *) NULL, 1);
	}

    }				/* Pass loop */
}
