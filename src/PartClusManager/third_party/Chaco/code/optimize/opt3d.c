/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	<math.h>
#include	"structs.h"
#include	"defs.h"


void      opt3d(graph, yvecs, nvtxs, nmyvtxs, vwsqrt, ptheta, pphi, pgamma, using_vwgts)
struct vtx_data **graph;	/* data structure containing vertex weights */
double  **yvecs;		/* eigenvectors */
int       nvtxs;		/* total number of vertices */
int       nmyvtxs;		/* number of vertices I own */
double   *vwsqrt;		/* square root of vertex weights */
double   *ptheta, *pphi, *pgamma;	/* return optimal angles */
int       using_vwgts;		/* are vertex weights being used? */

/* Compute rotation angle to minimize distance to discrete points. */
{
    extern int DEBUG_OPTIMIZE;	/* debug flag for optimization */
    extern int OPT3D_NTRIES;	/* number of local opts to find global min */
    double   *aptr, *bptr, *cptr;	/* loop through yvecs */
    double   *wsptr;		/* loops through vwsqrt */
    double    coeffs[25];	/* various products of yvecs */
    double    vars[3];		/* angular variables */
    double    best[3];		/* best minimizer found so far */
    double    grad[3];		/* gradiant of the function */
    double    gradc[3];		/* gradiant of the constraint */
    double    hess[3][3];	/* hessian of the function */
    double    hessc[3][3];	/* hessian of the constraint */
    double    step[3];		/* Newton step in optimization */
    double    grad_norm;	/* norm of the gradient */
    double    grad_min;		/* acceptable gradient for convergence */
    double    a, b, c;		/* temporary values */
    double    funcf, funcc;	/* values of function to be minimized */
    double    step_size;	/* norm of step */
    double    step_max;		/* maximum allowed step */
    double    step_min;		/* minimum step => convergence */
    double    early_step_min;	/* min step for early convergence stages */
    double    final_step_min;	/* min step for final convergence */
    double    hess_min;		/* value for hessian if < 0 */
    double    hess_tol;		/* smallest possible positive hess_min */
    double    hfact;		/* scales minimum tolerated hessian */
    double    w, ws;		/* vertex weight squared or to the 1.5 */
    double    mult;		/* multiplier for constraint violation */
    double    max_constraint;	/* maximum allowed value for constraint */
    double    eval;		/* smallest eigenvalue of Hessian */
    double    pdtol;		/* eval < tol considered to be 0 */
    double    mfactor;		/* scaling for constraint growth */
    double    mstart;		/* starting value for constraint scaling */
    double    bestf;		/* value of best minimizer so far */
    double    res;		/* returned eigen-residual */
    int       pdflag;		/* converging to non-minimum? */
    int       inner;		/* number of iterations at each stage */
    int       inner1;
    int       total;		/* total number of iterations */
    int       ntries, maxtries;	/* number of local minimizations */
    int       i, j;		/* loop counter */
int kk;
    double    func3d(), constraint();
    double    drandom();
    void      grad3d(), hess3d(), gradcon(), hesscon(), kramer3(), eigenvec3();
    void      evals3();

    /* Set parameters. */
    a = sqrt((double) nvtxs);
    step_max = PI / 4;
    early_step_min = 2.0e-4;
    final_step_min = early_step_min / 10;
    grad_min = 1.0e-7;
    hfact = 2;
    hess_tol = 1.0e-6;
    pdtol = 1.0e-7;
    max_constraint = 1.0e-12 * a;
    mfactor = 20.0;
    mstart = 5.0 * a;

    for (i = 0; i < 25; i++)
	coeffs[i] = 0;

    aptr = yvecs[1] + 1;
    bptr = yvecs[2] + 1;
    cptr = yvecs[3] + 1;
    wsptr = vwsqrt + 1;
    for (i = 1; i <= nmyvtxs; i++) {
	a = *aptr++;
	b = *bptr++;
	c = *cptr++;
	w = graph[i]->vwgt;
	if (using_vwgts)
	    ws = *wsptr++;
	if (w == 1) {
	    coeffs[0] += a * a * a * a;
	    coeffs[1] += b * b * b * b;
	    coeffs[2] += c * c * c * c;
	    coeffs[3] += a * a * a * b;
	    coeffs[4] += a * a * b * b;
	    coeffs[5] += a * b * b * b;
	    coeffs[6] += a * a * a * c;
	    coeffs[7] += a * a * c * c;
	    coeffs[8] += a * c * c * c;
	    coeffs[9] += b * b * b * c;
	    coeffs[10] += b * b * c * c;
	    coeffs[11] += b * c * c * c;
	    coeffs[12] += a * a * b * c;
	    coeffs[13] += a * b * b * c;
	    coeffs[14] += a * b * c * c;

	    coeffs[15] += a * a * a;
	    coeffs[16] += b * b * b;
	    coeffs[17] += c * c * c;
	    coeffs[18] += a * a * b;
	    coeffs[19] += a * a * c;
	    coeffs[20] += a * b * b;
	    coeffs[21] += b * b * c;
	    coeffs[22] += a * c * c;
	    coeffs[23] += b * c * c;
	    coeffs[24] += a * b * c;
	}
	else {
	    w = 1 / (w * w);
	    ws = 1 / ws;
	    coeffs[0] += a * a * a * a * w;
	    coeffs[1] += b * b * b * b * w;
	    coeffs[2] += c * c * c * c * w;
	    coeffs[3] += a * a * a * b * w;
	    coeffs[4] += a * a * b * b * w;
	    coeffs[5] += a * b * b * b * w;
	    coeffs[6] += a * a * a * c * w;
	    coeffs[7] += a * a * c * c * w;
	    coeffs[8] += a * c * c * c * w;
	    coeffs[9] += b * b * b * c * w;
	    coeffs[10] += b * b * c * c * w;
	    coeffs[11] += b * c * c * c * w;
	    coeffs[12] += a * a * b * c * w;
	    coeffs[13] += a * b * b * c * w;
	    coeffs[14] += a * b * c * c * w;

	    coeffs[15] += a * a * a * ws;
	    coeffs[16] += b * b * b * ws;
	    coeffs[17] += c * c * c * ws;
	    coeffs[18] += a * a * b * ws;
	    coeffs[19] += a * a * c * ws;
	    coeffs[20] += a * b * b * ws;
	    coeffs[21] += b * b * c * ws;
	    coeffs[22] += a * c * c * ws;
	    coeffs[23] += b * c * c * ws;
	    coeffs[24] += a * b * c * ws;
	}
    }

    /* Adjust for normalization of eigenvectors. */
    /* This should make convergence criteria insensitive to problem size. */
    /* Note that the relative sizes of funcf and funcc depend on normalization of
       eigenvectors, and I'm assuming them normalized to 1. */
    for (i = 0; i < 15; i++)
	coeffs[i] *= nvtxs;
    a = sqrt((double) nvtxs);
    for (i = 15; i < 25; i++)
	coeffs[i] *= a;

    bestf = 0;
    maxtries = OPT3D_NTRIES;
    for (ntries = 1; ntries <= maxtries; ntries++) {
	/* Initialize the starting guess randomly. */
	vars[0] = TWOPI * (drandom() - .5);
	vars[1] = acos(2.0 * drandom() - 1.0) - HALFPI;
	vars[2] = TWOPI * (drandom() - .5);

	inner1 = 0;
	total = 0;
	mult = mstart;
	step_min = early_step_min;
	funcc = max_constraint;
	while (funcc >= max_constraint && total < 70) {
	    inner = 0;
	    step_size = step_min;
	    pdflag = FALSE;
	    grad_norm = 0;
	    while (step_size >= step_min && (!pdflag || grad_norm > grad_min)
		   && inner < 15) {
		funcf = func3d(coeffs, vars[0], vars[1], vars[2]);
		grad3d(coeffs, grad, vars[0], vars[1], vars[2]);
		hess3d(coeffs, hess);

		/* Compute contribution of constraint term. */
		funcc = constraint(&coeffs[15]);
		/* func = funcf + mult*funcc; */
		gradcon(&coeffs[15], gradc);
		hesscon(&coeffs[15], hessc);

		/* If in final pass, tighten convergence criterion. */
		if (funcc < max_constraint)
		    step_min = final_step_min;

kk = 0;
if (kk) {
  evals3(hessc, &eval, &res, &res);
}
   

		for (i = 0; i < 3; i++) {
		    /* Note: I'm taking negative of gradient here. */
		    grad[i] = -grad[i] - mult * gradc[i];
		    for (j = 0; j < 3; j++)
			hess[i][j] += mult * hessc[i][j];
		}

		grad_norm = fabs(grad[0]) + fabs(grad[1]) + fabs(grad[2]);
		hess_min = hfact * grad_norm / step_max;
		if (hess_min < hess_tol)
		    hess_min = hess_tol;

		/* Find smallest eigenvalue of hess. */
		evals3(hess, &eval, &res, &res);

		/* If eval < 0, add to diagonal to make pos def. */
		if (eval < -pdtol)
		    pdflag = FALSE;
		else
		    pdflag = TRUE;

		if (eval < hess_min) {
		    for (i = 0; i < 3; i++)
			hess[i][i] += hess_min - eval;
		}

		/* Now solve linear system for step sizes. */
		kramer3(hess, grad, step);

		/* Scale step down if too big. */
		step_size = fabs(step[0]) + fabs(step[1]) + fabs(step[2]);
		if (step_size > step_max) {
		    a = step_max / step_size;
		    for (i = 0; i < 3; i++)
			step[i] *= a;
		}

		if ((step_size < step_min || grad_norm < grad_min) && !pdflag) {
		    /* Convergence to non-min. */
		    for (i = 0; i < 3; i++)
			hess[i][i] -= hess_min - eval;
		    eigenvec3(hess, eval, step, &res);
		    step_size = fabs(step[0]) + fabs(step[1]) + fabs(step[2]);
		    a = step_min / step_size;
		    for (i = 0; i < 3; i++)
			step[i] *= a;
		    step_size = step_min;
		}
		for (i = 0; i < 3; i++)
		    vars[i] += step[i];
		inner++;
	    }
	    if (inner1 == 0)
		inner1 = inner;
	    total += inner;
	    mult *= mfactor;
	}

	if (DEBUG_OPTIMIZE > 0) {
	    printf("On try %d, After %d (%d) passes, funcf=%e, funcc=%e (%f, %f, %f)\n",
		   ntries, total, inner1, funcf, funcc, vars[0], vars[1], vars[2]);
	}

	if (ntries == 1 || funcf < bestf) {
	    bestf = funcf;
	    for (i = 0; i < 3; i++)
		best[i] = vars[i];
	}
    }
    *ptheta = best[0];
    *pphi = best[1];
    *pgamma = best[2];
}
