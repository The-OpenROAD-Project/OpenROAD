/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	<math.h>
#include	"defs.h"
#include	"structs.h"


double    opt2d(graph, yvecs, nvtxs, nmyvtxs)
/* Compute rotation angle to minimize distance to discrete points. */
struct vtx_data **graph;	/* data structure with vertex weights */
double  **yvecs;		/* eigenvectors */
int       nvtxs;		/* total number of vertices */
int       nmyvtxs;		/* number of vertices I own */
{
    extern int DEBUG_OPTIMIZE;	/* debug flag for optimization */
    double   *aptr, *bptr;	/* loop through yvecs */
    double    coeffs[5];	/* various products of yvecs */
    double    a, b;		/* temporary values */
    double    func;		/* value of function to be minimized */
    double    grad, hess;	/* first and 2nd derivatives of function */
    double    grad_min;		/* acceptably small gradient */
    double    theta;		/* angle being optimized */
    double    step;		/* change in angle */
    double    step_max;		/* maximum allowed step */
    double    step_min;		/* minimum step => convergence */
    double    hess_min;		/* value for hessian is < 0 */
    double    hfact;		/* scaling for min tolerated hessian */
    double    w;		/* vertex weight squared */
    double    pdtol;		/* allowed error in hessian pd-ness */
    int       pdflag;		/* is hessian positive semi-definite? */
    int       i;		/* loop counter */
    double    func2d();
    double    grad2d();
    double    hess2d();

    /* Set parameters. */
    step_max = PI / 8;
    step_min = 2.0e-5;
    grad_min = 1.0e-7;
    pdtol = 1.0e-8;
    hfact = 2;

    for (i = 0; i < 5; i++)
	coeffs[i] = 0;
    aptr = yvecs[1] + 1;
    bptr = yvecs[2] + 1;
    for (i = 1; i <= nmyvtxs; i++) {
	a = *aptr++;
	b = *bptr++;
	w = graph[i]->vwgt;
	if (w == 1) {
	    coeffs[0] += a * a * a * a;
	    coeffs[1] += a * a * a * b;
	    coeffs[2] += a * a * b * b;
	    coeffs[3] += a * b * b * b;
	    coeffs[4] += b * b * b * b;
	}
	else {
	    w = 1 / (w * w);
	    coeffs[0] += a * a * a * a * w;
	    coeffs[1] += a * a * a * b * w;
	    coeffs[2] += a * a * b * b * w;
	    coeffs[3] += a * b * b * b * w;
	    coeffs[4] += b * b * b * b * w;
	}
    }
    /* Adjust for normalization of eigenvectors. */
    /* This should make tolerances independent of vector length */
    for (i = 0; i < 5; i++)
	coeffs[i] *= nvtxs;

    i = 0;
    theta = 0.0;
    step = step_max;
    pdflag = FALSE;
    grad = 0;
    while (fabs(step) >= step_min && (!pdflag || fabs(grad) > grad_min)) {
	func = func2d(coeffs, theta);
	grad = grad2d(coeffs, theta);
	hess = hess2d(coeffs);

	if (hess < -pdtol)
	    pdflag = FALSE;
	else
	    pdflag = TRUE;

	hess_min = hfact * fabs(grad) / step_max;
	if (hess < hess_min)
	    hess = hess_min;

	if (fabs(grad) > fabs(hess * step_max))
	    step = -step_max * sign(grad);
	else
	    step = -grad / hess;

	theta += step;
	if (fabs(step) < step_min && !pdflag) {	/* Convergence to non-min. */
	    step = step_min;
	    theta += step;
	}
	i++;
    }

    if (DEBUG_OPTIMIZE > 0) {
	printf("After %d passes, func=%e, theta = %f\n", i, func, theta);
    }

    return (theta);
}
