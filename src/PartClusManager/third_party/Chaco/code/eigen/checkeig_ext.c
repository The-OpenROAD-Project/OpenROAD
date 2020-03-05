/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include "defs.h"
#include "structs.h"

/* Check an extended eigenpair of A by direct multiplication. Uses
   the Ay = extval*y + Dg form of the problem for convenience. */

double    checkeig_ext(err, work, A, y, n, extval, vwsqrt, gvec, eigtol, warnings)
double   *err;
double   *work;			/* work vector of length n */
struct vtx_data **A;
double   *y;
int       n;
double    extval;
double   *vwsqrt;
double   *gvec;
double    eigtol;
int       warnings;		/* don't want to see warning messages in one of the
				   contexts this is called */
{
    extern FILE *Output_File;	/* output file or null */
    extern int DEBUG_EVECS;	/* print debugging output? */
    extern int WARNING_EVECS;	/* print warning messages? */
    double    resid;		/* the extended eigen residual */
    double    norm();		/* vector norm */
    void      splarax();	/* sparse matrix vector mult */
    void      scadd();		/* scaled vector add */
    void      scale_diag();	/* scale vector by another's elements */
    void      cpvec();		/* vector copy */

    splarax(err, A, n, y, vwsqrt, work);
    scadd(err, 1, n, -extval, y);
    cpvec(work, 1, n, gvec);	/* only need if going to re-use gvec */
    scale_diag(work, 1, n, vwsqrt);
    scadd(err, 1, n, -1.0, work);
    resid = norm(err, 1, n);

    if (DEBUG_EVECS > 0) {
	printf("  extended residual: %g\n", resid);
	if (Output_File != NULL) {
	    fprintf(Output_File, "  extended residual: %g\n", resid);
	}
    }
    if (warnings && WARNING_EVECS > 0 && resid > eigtol) {
	printf("WARNING: Extended residual (%g) greater than tolerance (%g).\n",
	       resid, eigtol);
	if (Output_File != NULL) {
	    fprintf(Output_File,
		  "WARNING: Extended residual (%g) greater than tolerance (%g).\n",
		    resid, eigtol);
	}
    }

    return (resid);
}
