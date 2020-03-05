/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	<math.h>
#include	"defs.h"

static double stheta, ctheta;	/* sign and cosine of angle */
static double sphi, cphi;	/* sign and cosine of angle */
static double sgamma, cgamma;	/* squares of sign and cosine of angle */
static double onemcg;		/* intermediate value */

static double a1, a2, a3;	/* linear mapping coefficients */
static double b1, b2, b3;	/* linear mapping coefficients */
static double c1, c2, c3;	/* linear mapping coefficients */

static double a1t, a2t, a3t;	/* theta first derivatives */
static double b1t, b2t, b3t;	/* theta first derivatives */
static double c1t, c2t, c3t;	/* theta first derivatives */

static double a1p, a2p, a3p;	/* phi first derivatives */
static double b1p, b2p, b3p;	/* phi first derivatives */
static double c1p, c2p, c3p;	/* phi first derivatives */

static double a1g, a2g, a3g;	/* gamma first derivatives */
static double b1g, b2g, b3g;	/* gamma first derivatives */
static double c1g, c2g, c3g;	/* gamma first derivatives */

static double a1tt, a2tt, a3tt;	/* theta/theta second derivatives */
static double b1tt, b2tt, b3tt;	/* theta/theta second derivatives */
static double c1tt, c2tt, c3tt;	/* theta/theta second derivatives */
static double a1pp, a2pp, a3pp;	/* phi/phi second derivatives */
static double b1pp, b2pp, b3pp;	/* phi/phi second derivatives */
static double c1pp, c2pp, c3pp;	/* phi/phi second derivatives */
static double a1gg, a2gg, a3gg;	/* gamma/gamma second derivatives */
static double b1gg, b2gg, b3gg;	/* gamma/gamma second derivatives */
static double c1gg, c2gg, c3gg;	/* gamma/gamma second derivatives */
static double a1tp, a2tp, a3tp;	/* theta/phi second derivatives */
static double b1tp, b2tp, b3tp;	/* theta/phi second derivatives */
static double c1tp, c2tp, c3tp;	/* theta/phi second derivatives */
static double a1tg, a2tg, a3tg;	/* theta/gamma second derivatives */
static double b1tg, b2tg, b3tg;	/* theta/gamma second derivatives */
static double c1tg, c2tg, c3tg;	/* theta/gamma second derivatives */
static double a1pg, a2pg, a3pg;	/* phi/gamma second derivatives */
static double b1pg, b2pg, b3pg;	/* phi/gamma second derivatives */
static double c1pg, c2pg, c3pg;	/* phi/gamma second derivatives */

static double cval;		/* value of constraint equation */
static double cgrad[3];		/* grad of constraint equation */


double    func3d(coeffs, theta, phi, gamma2)
double    coeffs[15];		/* five different 4-way products */
double    theta, phi, gamma2;	/* angular parameters */

/* Returns value of penalty function at given angle. */
{
    double    val;		/* functional value */

    stheta = sin(theta);
    ctheta = cos(theta);
    sphi = sin(phi);
    cphi = cos(phi);
    sgamma = sin(gamma2);
    cgamma = cos(gamma2);
    onemcg = 1.0 - cgamma;

/* R <- R*cgamma + v(R.v)(1-cgamma) - (R x v)sgamma */
/*

   vx = cphi*(ctheta*offset[0] + stheta*offset[1]) + sphi*offset[2];

   vx *= onemcg;

   pos[0] = cgamma*offset[0] +
	    cphi*ctheta*vx +
            sgamma*(-sphi*offset[1] + cphi*stheta*offset[2]);

   pos[1] = cgamma*offset[1] + cphi*stheta*vx +
	    sgamma*(sphi*offset[0] - cphi*ctheta*offset[2]);

   pos[2] = cgamma*offset[2] +
	    sphi*vx +
	    sgamma*cphi*(-stheta*offset[0] + ctheta*offset[1]);
*/

    a1 = cgamma + cphi * ctheta * onemcg * cphi * ctheta;
    a2 = sgamma * sphi + cphi * stheta * onemcg * cphi * ctheta;
    a3 = -sgamma * cphi * stheta + sphi * onemcg * cphi * ctheta;

    b1 = -sgamma * sphi + cphi * ctheta * onemcg * cphi * stheta;
    b2 = cgamma + cphi * stheta * onemcg * cphi * stheta;
    b3 = sgamma * cphi * ctheta + sphi * onemcg * cphi * stheta;

    c1 = sgamma * cphi * stheta + cphi * ctheta * onemcg * sphi;
    c2 = -sgamma * cphi * ctheta + cphi * stheta * onemcg * sphi;
    c3 = cgamma + sphi * onemcg * sphi;

    val = (a1 * a1 * a1 * a1 + a2 * a2 * a2 * a2 + a3 * a3 * a3 * a3) * coeffs[0];
    val += (b1 * b1 * b1 * b1 + b2 * b2 * b2 * b2 + b3 * b3 * b3 * b3) * coeffs[1];
    val += (c1 * c1 * c1 * c1 + c2 * c2 * c2 * c2 + c3 * c3 * c3 * c3) * coeffs[2];
    val += 4 * (a1 * a1 * a1 * b1 + a2 * a2 * a2 * b2 + a3 * a3 * a3 * b3) * coeffs[3];
    val += 6 * (a1 * a1 * b1 * b1 + a2 * a2 * b2 * b2 + a3 * a3 * b3 * b3) * coeffs[4];
    val += 4 * (a1 * b1 * b1 * b1 + a2 * b2 * b2 * b2 + a3 * b3 * b3 * b3) * coeffs[5];
    val += 4 * (a1 * a1 * a1 * c1 + a2 * a2 * a2 * c2 + a3 * a3 * a3 * c3) * coeffs[6];
    val += 6 * (a1 * a1 * c1 * c1 + a2 * a2 * c2 * c2 + a3 * a3 * c3 * c3) * coeffs[7];
    val += 4 * (a1 * c1 * c1 * c1 + a2 * c2 * c2 * c2 + a3 * c3 * c3 * c3) * coeffs[8];
    val += 4 * (b1 * b1 * b1 * c1 + b2 * b2 * b2 * c2 + b3 * b3 * b3 * c3) * coeffs[9];
    val += 6 * (b1 * b1 * c1 * c1 + b2 * b2 * c2 * c2 + b3 * b3 * c3 * c3) * coeffs[10];
    val += 4 * (b1 * c1 * c1 * c1 + b2 * c2 * c2 * c2 + b3 * c3 * c3 * c3) * coeffs[11];
    val += 12 * (a1 * a1 * b1 * c1 + a2 * a2 * b2 * c2 + a3 * a3 * b3 * c3) * coeffs[12];
    val += 12 * (a1 * b1 * b1 * c1 + a2 * b2 * b2 * c2 + a3 * b3 * b3 * c3) * coeffs[13];
    val += 12 * (a1 * b1 * c1 * c1 + a2 * b2 * c2 * c2 + a3 * b3 * c3 * c3) * coeffs[14];

    return (val);
}


static double grad0(), grad1(), grad2();

void      grad3d(coeffs, grad, theta, phi, gamma2)
double    coeffs[15];		/* five different 4-way products */
double    grad[3];		/* gradient returned */
double    theta, phi, gamma2;	/* angular parameters */

/* Returns 1st derivative of penalty function at given angle. */
{

    stheta = sin(theta);
    ctheta = cos(theta);
    sphi = sin(phi);
    cphi = cos(phi);
    sgamma = sin(gamma2);
    cgamma = cos(gamma2);
    onemcg = 1.0 - cgamma;

    a1 = cgamma + ctheta * ctheta * cphi * cphi * onemcg;
    a2 = sphi * sgamma + stheta * ctheta * cphi * cphi * onemcg;
    a3 = -stheta * cphi * sgamma + ctheta * sphi * cphi * onemcg;

    b1 = -sphi * sgamma + stheta * ctheta * cphi * cphi * onemcg;
    b2 = cgamma + stheta * stheta * cphi * cphi * onemcg;
    b3 = ctheta * cphi * sgamma + stheta * sphi * cphi * onemcg;

    c1 = stheta * cphi * sgamma + ctheta * cphi * sphi * onemcg;
    c2 = -ctheta * cphi * sgamma + stheta * cphi * sphi * onemcg;
    c3 = cgamma + sphi * sphi * onemcg;


    /* Order = theta, phi, gamma. */
    a1t = -2 * ctheta * stheta * cphi * cphi * onemcg;
    a2t = (ctheta * ctheta - stheta * stheta) * cphi * cphi * onemcg;
    a3t = -sgamma * cphi * ctheta - stheta * sphi * cphi * onemcg;

    b1t = (ctheta * ctheta - stheta * stheta) * cphi * cphi * onemcg;
    b2t = 2 * stheta * ctheta * cphi * cphi * onemcg;
    b3t = -stheta * sgamma * cphi + ctheta * sphi * cphi * onemcg;

    c1t = ctheta * sgamma * cphi - stheta * cphi * sphi * onemcg;
    c2t = stheta * sgamma * cphi + ctheta * cphi * sphi * onemcg;
    c3t = 0;

    grad[0] = grad0(coeffs);

    a1p = -2 * ctheta * ctheta * cphi * sphi * onemcg;
    a2p = sgamma * cphi - 2 * cphi * sphi * stheta * ctheta * onemcg;
    a3p = sgamma * sphi * stheta + (cphi * cphi - sphi * sphi) * ctheta * onemcg;

    b1p = -sgamma * cphi - 2 * cphi * sphi * ctheta * stheta * onemcg;
    b2p = -2 * cphi * sphi * stheta * stheta * onemcg;
    b3p = -ctheta * sgamma * sphi + (cphi * cphi - sphi * sphi) * stheta * onemcg;

    c1p = -stheta * sgamma * sphi + (cphi * cphi - sphi * sphi) * ctheta * onemcg;
    c2p = ctheta * sgamma * sphi + (cphi * cphi - sphi * sphi) * stheta * onemcg;
    c3p = 2 * sphi * cphi * onemcg;

    grad[1] = grad1(coeffs);

    a1g = -sgamma + ctheta * ctheta * cphi * cphi * sgamma;
    a2g = cgamma * sphi + cphi * cphi * stheta * ctheta * sgamma;
    a3g = -cgamma * cphi * stheta + cphi * sphi * ctheta * sgamma;

    b1g = -cgamma * sphi + cphi * cphi * ctheta * stheta * sgamma;
    b2g = -sgamma + cphi * cphi * stheta * stheta * sgamma;
    b3g = ctheta * cgamma * cphi + sphi * cphi * stheta * sgamma;

    c1g = stheta * cgamma * cphi + cphi * sphi * ctheta * sgamma;
    c2g = -ctheta * cgamma * cphi + cphi * sphi * stheta * sgamma;
    c3g = -sgamma + sphi * sphi * sgamma;

    grad[2] = grad2(coeffs);
}

static double grad0(coeffs)
double   *coeffs;
{
    double    val;

    val = 4 * (a1t * a1 * a1 * a1 + a2t * a2 * a2 * a2 + a3t * a3 * a3 * a3) * coeffs[0];
    val += 4 * (b1t * b1 * b1 * b1 + b2t * b2 * b2 * b2 + b3t * b3 * b3 * b3) * coeffs[1];
    val += 4 * (c1t * c1 * c1 * c1 + c2t * c2 * c2 * c2 + c3t * c3 * c3 * c3) * coeffs[2];
    val += (12 * (a1t * a1 * a1 * b1 + a2t * a2 * a2 * b2 + a3t * a3 * a3 * b3) +
    4 * (a1 * a1 * a1 * b1t + a2 * a2 * a2 * b2t + a3 * a3 * a3 * b3t)) * coeffs[3];
    val += (12 * (a1t * a1 * b1 * b1 + a2t * a2 * b2 * b2 + a3t * a3 * b3 * b3) +
	    12 * (a1 * a1 * b1t * b1 + a2 * a2 * b2t * b2 + a3 * a3 * b3t * b3)) * coeffs[4];
    val += (4 * (a1t * b1 * b1 * b1 + a2t * b2 * b2 * b2 + a3t * b3 * b3 * b3) +
	    12 * (a1 * b1t * b1 * b1 + a2 * b2t * b2 * b2 + a3 * b3t * b3 * b3)) * coeffs[5];
    val += (12 * (a1t * a1 * a1 * c1 + a2t * a2 * a2 * c2 + a3t * a3 * a3 * c3) +
    4 * (a1 * a1 * a1 * c1t + a2 * a2 * a2 * c2t + a3 * a3 * a3 * c3t)) * coeffs[6];
    val += (12 * (a1t * a1 * c1 * c1 + a2t * a2 * c2 * c2 + a3t * a3 * c3 * c3) +
	    12 * (a1 * a1 * c1t * c1 + a2 * a2 * c2t * c2 + a3 * a3 * c3t * c3)) * coeffs[7];
    val += (4 * (a1t * c1 * c1 * c1 + a2t * c2 * c2 * c2 + a3t * c3 * c3 * c3) +
	    12 * (a1 * c1t * c1 * c1 + a2 * c2t * c2 * c2 + a3 * c3t * c3 * c3)) * coeffs[8];
    val += (12 * (b1t * b1 * b1 * c1 + b2t * b2 * b2 * c2 + b3t * b3 * b3 * c3) +
    4 * (b1 * b1 * b1 * c1t + b2 * b2 * b2 * c2t + b3 * b3 * b3 * c3t)) * coeffs[9];
    val += (12 * (b1t * b1 * c1 * c1 + b2t * b2 * c2 * c2 + b3t * b3 * c3 * c3) +
	    12 * (b1 * b1 * c1t * c1 + b2 * b2 * c2t * c2 + b3 * b3 * c3t * c3)) * coeffs[10];
    val += (4 * (b1t * c1 * c1 * c1 + b2t * c2 * c2 * c2 + b3t * c3 * c3 * c3) +
	    12 * (b1 * c1t * c1 * c1 + b2 * c2t * c2 * c2 + b3 * c3t * c3 * c3)) * coeffs[11];
    val += (24 * (a1t * a1 * b1 * c1 + a2t * a2 * b2 * c2 + a3t * a3 * b3 * c3) +
	    12 * (a1 * a1 * b1t * c1 + a2 * a2 * b2t * c2 + a3 * a3 * b3t * c3) +
	    12 * (a1 * a1 * b1 * c1t + a2 * a2 * b2 * c2t + a3 * a3 * b3 * c3t)) * coeffs[12];
    val += (12 * (a1t * b1 * b1 * c1 + a2t * b2 * b2 * c2 + a3t * b3 * b3 * c3) +
	    24 * (a1 * b1t * b1 * c1 + a2 * b2t * b2 * c2 + a3 * b3t * b3 * c3) +
	    12 * (a1 * b1 * b1 * c1t + a2 * b2 * b2 * c2t + a3 * b3 * b3 * c3t)) * coeffs[13];
    val += (12 * (a1t * b1 * c1 * c1 + a2t * b2 * c2 * c2 + a3t * b3 * c3 * c3) +
	    12 * (a1 * b1t * c1 * c1 + a2 * b2t * c2 * c2 + a3 * b3t * c3 * c3) +
	    24 * (a1 * b1 * c1t * c1 + a2 * b2 * c2t * c2 + a3 * b3 * c3t * c3)) * coeffs[14];
    return (val);
}

static double grad1(coeffs)
double   *coeffs;
{
    double    val;

    val = 4 * (a1p * a1 * a1 * a1 + a2p * a2 * a2 * a2 + a3p * a3 * a3 * a3) * coeffs[0];
    val += 4 * (b1p * b1 * b1 * b1 + b2p * b2 * b2 * b2 + b3p * b3 * b3 * b3) * coeffs[1];
    val += 4 * (c1p * c1 * c1 * c1 + c2p * c2 * c2 * c2 + c3p * c3 * c3 * c3) * coeffs[2];
    val += (12 * (a1p * a1 * a1 * b1 + a2p * a2 * a2 * b2 + a3p * a3 * a3 * b3) +
    4 * (a1 * a1 * a1 * b1p + a2 * a2 * a2 * b2p + a3 * a3 * a3 * b3p)) * coeffs[3];
    val += (12 * (a1p * a1 * b1 * b1 + a2p * a2 * b2 * b2 + a3p * a3 * b3 * b3) +
	    12 * (a1 * a1 * b1p * b1 + a2 * a2 * b2p * b2 + a3 * a3 * b3p * b3)) * coeffs[4];
    val += (4 * (a1p * b1 * b1 * b1 + a2p * b2 * b2 * b2 + a3p * b3 * b3 * b3) +
	    12 * (a1 * b1p * b1 * b1 + a2 * b2p * b2 * b2 + a3 * b3p * b3 * b3)) * coeffs[5];
    val += (12 * (a1p * a1 * a1 * c1 + a2p * a2 * a2 * c2 + a3p * a3 * a3 * c3) +
    4 * (a1 * a1 * a1 * c1p + a2 * a2 * a2 * c2p + a3 * a3 * a3 * c3p)) * coeffs[6];
    val += (12 * (a1p * a1 * c1 * c1 + a2p * a2 * c2 * c2 + a3p * a3 * c3 * c3) +
	    12 * (a1 * a1 * c1p * c1 + a2 * a2 * c2p * c2 + a3 * a3 * c3p * c3)) * coeffs[7];
    val += (4 * (a1p * c1 * c1 * c1 + a2p * c2 * c2 * c2 + a3p * c3 * c3 * c3) +
	    12 * (a1 * c1p * c1 * c1 + a2 * c2p * c2 * c2 + a3 * c3p * c3 * c3)) * coeffs[8];
    val += (12 * (b1p * b1 * b1 * c1 + b2p * b2 * b2 * c2 + b3p * b3 * b3 * c3) +
    4 * (b1 * b1 * b1 * c1p + b2 * b2 * b2 * c2p + b3 * b3 * b3 * c3p)) * coeffs[9];
    val += (12 * (b1p * b1 * c1 * c1 + b2p * b2 * c2 * c2 + b3p * b3 * c3 * c3) +
	    12 * (b1 * b1 * c1p * c1 + b2 * b2 * c2p * c2 + b3 * b3 * c3p * c3)) * coeffs[10];
    val += (4 * (b1p * c1 * c1 * c1 + b2p * c2 * c2 * c2 + b3p * c3 * c3 * c3) +
	    12 * (b1 * c1p * c1 * c1 + b2 * c2p * c2 * c2 + b3 * c3p * c3 * c3)) * coeffs[11];
    val += (24 * (a1p * a1 * b1 * c1 + a2p * a2 * b2 * c2 + a3p * a3 * b3 * c3) +
	    12 * (a1 * a1 * b1p * c1 + a2 * a2 * b2p * c2 + a3 * a3 * b3p * c3) +
	    12 * (a1 * a1 * b1 * c1p + a2 * a2 * b2 * c2p + a3 * a3 * b3 * c3p)) * coeffs[12];
    val += (12 * (a1p * b1 * b1 * c1 + a2p * b2 * b2 * c2 + a3p * b3 * b3 * c3) +
	    24 * (a1 * b1p * b1 * c1 + a2 * b2p * b2 * c2 + a3 * b3p * b3 * c3) +
	    12 * (a1 * b1 * b1 * c1p + a2 * b2 * b2 * c2p + a3 * b3 * b3 * c3p)) * coeffs[13];
    val += (12 * (a1p * b1 * c1 * c1 + a2p * b2 * c2 * c2 + a3p * b3 * c3 * c3) +
	    12 * (a1 * b1p * c1 * c1 + a2 * b2p * c2 * c2 + a3 * b3p * c3 * c3) +
	    24 * (a1 * b1 * c1p * c1 + a2 * b2 * c2p * c2 + a3 * b3 * c3p * c3)) * coeffs[14];
    return (val);
}

static double grad2(coeffs)
double   *coeffs;
{
    double    val;

    val = 4 * (a1g * a1 * a1 * a1 + a2g * a2 * a2 * a2 + a3g * a3 * a3 * a3) * coeffs[0];
    val += 4 * (b1g * b1 * b1 * b1 + b2g * b2 * b2 * b2 + b3g * b3 * b3 * b3) * coeffs[1];
    val += 4 * (c1g * c1 * c1 * c1 + c2g * c2 * c2 * c2 + c3g * c3 * c3 * c3) * coeffs[2];
    val += (12 * (a1g * a1 * a1 * b1 + a2g * a2 * a2 * b2 + a3g * a3 * a3 * b3) +
    4 * (a1 * a1 * a1 * b1g + a2 * a2 * a2 * b2g + a3 * a3 * a3 * b3g)) * coeffs[3];
    val += (12 * (a1g * a1 * b1 * b1 + a2g * a2 * b2 * b2 + a3g * a3 * b3 * b3) +
	    12 * (a1 * a1 * b1g * b1 + a2 * a2 * b2g * b2 + a3 * a3 * b3g * b3)) * coeffs[4];
    val += (4 * (a1g * b1 * b1 * b1 + a2g * b2 * b2 * b2 + a3g * b3 * b3 * b3) +
	    12 * (a1 * b1g * b1 * b1 + a2 * b2g * b2 * b2 + a3 * b3g * b3 * b3)) * coeffs[5];
    val += (12 * (a1g * a1 * a1 * c1 + a2g * a2 * a2 * c2 + a3g * a3 * a3 * c3) +
    4 * (a1 * a1 * a1 * c1g + a2 * a2 * a2 * c2g + a3 * a3 * a3 * c3g)) * coeffs[6];
    val += (12 * (a1g * a1 * c1 * c1 + a2g * a2 * c2 * c2 + a3g * a3 * c3 * c3) +
	    12 * (a1 * a1 * c1g * c1 + a2 * a2 * c2g * c2 + a3 * a3 * c3g * c3)) * coeffs[7];
    val += (4 * (a1g * c1 * c1 * c1 + a2g * c2 * c2 * c2 + a3g * c3 * c3 * c3) +
	    12 * (a1 * c1g * c1 * c1 + a2 * c2g * c2 * c2 + a3 * c3g * c3 * c3)) * coeffs[8];
    val += (12 * (b1g * b1 * b1 * c1 + b2g * b2 * b2 * c2 + b3g * b3 * b3 * c3) +
    4 * (b1 * b1 * b1 * c1g + b2 * b2 * b2 * c2g + b3 * b3 * b3 * c3g)) * coeffs[9];
    val += (12 * (b1g * b1 * c1 * c1 + b2g * b2 * c2 * c2 + b3g * b3 * c3 * c3) +
	    12 * (b1 * b1 * c1g * c1 + b2 * b2 * c2g * c2 + b3 * b3 * c3g * c3)) * coeffs[10];
    val += (4 * (b1g * c1 * c1 * c1 + b2g * c2 * c2 * c2 + b3g * c3 * c3 * c3) +
	    12 * (b1 * c1g * c1 * c1 + b2 * c2g * c2 * c2 + b3 * c3g * c3 * c3)) * coeffs[11];
    val += (24 * (a1g * a1 * b1 * c1 + a2g * a2 * b2 * c2 + a3g * a3 * b3 * c3) +
	    12 * (a1 * a1 * b1g * c1 + a2 * a2 * b2g * c2 + a3 * a3 * b3g * c3) +
	    12 * (a1 * a1 * b1 * c1g + a2 * a2 * b2 * c2g + a3 * a3 * b3 * c3g)) * coeffs[12];
    val += (12 * (a1g * b1 * b1 * c1 + a2g * b2 * b2 * c2 + a3g * b3 * b3 * c3) +
	    24 * (a1 * b1g * b1 * c1 + a2 * b2g * b2 * c2 + a3 * b3g * b3 * c3) +
	    12 * (a1 * b1 * b1 * c1g + a2 * b2 * b2 * c2g + a3 * b3 * b3 * c3g)) * coeffs[13];
    val += (12 * (a1g * b1 * c1 * c1 + a2g * b2 * c2 * c2 + a3g * b3 * c3 * c3) +
	    12 * (a1 * b1g * c1 * c1 + a2 * b2g * c2 * c2 + a3 * b3g * c3 * c3) +
	    24 * (a1 * b1 * c1g * c1 + a2 * b2 * c2g * c2 + a3 * b3 * c3g * c3)) * coeffs[14];
    return (val);
}


static double hess00(), hess11(), hess22(), hess01(), hess02(), hess12();

void      hess3d(coeffs, hess)
double    coeffs[15];		/* five different 4-way products */
double    hess[3][3];		/* Hessian returned */

/* Returns 2nd derivative of penalty function at given angle. */
{

    a1tt = -2 * (ctheta * ctheta - stheta * stheta) * cphi * cphi * onemcg;
    a2tt = -4 * ctheta * stheta * cphi * cphi * onemcg;
    a3tt = stheta * sgamma * cphi - ctheta * sphi * cphi * onemcg;

    b1tt = -4 * ctheta * stheta * cphi * cphi * onemcg;
    b2tt = 2 * (ctheta * ctheta - stheta * stheta) * cphi * cphi * onemcg;
    b3tt = -ctheta * sgamma * cphi - stheta * sphi * cphi * onemcg;

    c1tt = -stheta * sgamma * cphi - ctheta * cphi * sphi * onemcg;
    c2tt = ctheta * sgamma * cphi - stheta * cphi * sphi * onemcg;
    c3tt = 0;

    a1tp = 4 * ctheta * stheta * cphi * sphi * onemcg;
    a2tp = -2 * (ctheta * ctheta - stheta * stheta) * cphi * sphi * onemcg;
    a3tp = sgamma * sphi * ctheta - stheta * (cphi * cphi - sphi * sphi) * onemcg;

    b1tp = -2 * (ctheta * ctheta - stheta * stheta) * cphi * sphi * onemcg;
    b2tp = -4 * stheta * ctheta * cphi * sphi * onemcg;
    b3tp = stheta * sgamma * sphi + ctheta * (cphi * cphi - sphi * sphi) * onemcg;

    c1tp = -ctheta * sgamma * sphi - stheta * (cphi * cphi - sphi * sphi) * onemcg;
    c2tp = -stheta * sgamma * sphi + ctheta * (cphi * cphi - sphi * sphi) * onemcg;
    c3tp = 0;

    a1tg = -2 * ctheta * stheta * cphi * cphi * sgamma;
    a2tg = (ctheta * ctheta - stheta * stheta) * cphi * cphi * sgamma;
    a3tg = -cgamma * cphi * ctheta - stheta * sphi * cphi * sgamma;

    b1tg = (ctheta * ctheta - stheta * stheta) * cphi * cphi * sgamma;
    b2tg = 2 * stheta * ctheta * cphi * cphi * sgamma;
    b3tg = -stheta * cgamma * cphi + ctheta * sphi * cphi * sgamma;

    c1tg = ctheta * cgamma * cphi - stheta * cphi * sphi * sgamma;
    c2tg = stheta * cgamma * cphi + ctheta * cphi * sphi * sgamma;
    c3tg = 0;


    a1pp = -2 * ctheta * ctheta * (cphi * cphi - sphi * sphi) * onemcg;
    a2pp = -sgamma * sphi - 2 * (cphi * cphi - sphi * sphi) * stheta * ctheta * onemcg;
    a3pp = sgamma * cphi * stheta - 4 * cphi * sphi * ctheta * onemcg;

    b1pp = sgamma * sphi - 2 * (cphi * cphi - sphi * sphi) * ctheta * stheta * onemcg;
    b2pp = -2 * (cphi * cphi - sphi * sphi) * stheta * stheta * onemcg;
    b3pp = -ctheta * sgamma * cphi - 4 * cphi * sphi * stheta * onemcg;

    c1pp = -stheta * sgamma * cphi - 4 * cphi * sphi * ctheta * onemcg;
    c2pp = ctheta * sgamma * cphi - 4 * cphi * sphi * stheta * onemcg;
    c3pp = 2 * (cphi * cphi - sphi * sphi) * onemcg;


    a1pg = -2 * ctheta * ctheta * cphi * sphi * sgamma;
    a2pg = cgamma * cphi - 2 * cphi * sphi * stheta * ctheta * sgamma;
    a3pg = cgamma * sphi * stheta + (cphi * cphi - sphi * sphi) * ctheta * sgamma;

    b1pg = -cgamma * cphi - 2 * cphi * sphi * ctheta * stheta * sgamma;
    b2pg = -2 * cphi * sphi * stheta * stheta * sgamma;
    b3pg = -ctheta * cgamma * sphi + (cphi * cphi - sphi * sphi) * stheta * sgamma;

    c1pg = -stheta * cgamma * sphi + (cphi * cphi - sphi * sphi) * ctheta * sgamma;
    c2pg = ctheta * cgamma * sphi + (cphi * cphi - sphi * sphi) * stheta * sgamma;
    c3pg = 2 * sphi * cphi * sgamma;


    a1gg = -cgamma + ctheta * ctheta * cphi * cphi * cgamma;
    a2gg = -sgamma * sphi + cphi * cphi * stheta * ctheta * cgamma;
    a3gg = sgamma * cphi * stheta + cphi * sphi * ctheta * cgamma;

    b1gg = sgamma * sphi + cphi * cphi * ctheta * stheta * cgamma;
    b2gg = -cgamma + cphi * cphi * stheta * stheta * cgamma;
    b3gg = -ctheta * sgamma * cphi + sphi * cphi * stheta * cgamma;

    c1gg = -stheta * sgamma * cphi + cphi * sphi * ctheta * cgamma;
    c2gg = ctheta * sgamma * cphi + cphi * sphi * stheta * cgamma;
    c3gg = -cgamma + sphi * sphi * cgamma;

    hess[0][0] = hess00(coeffs);
    hess[1][1] = hess11(coeffs);
    hess[2][2] = hess22(coeffs);
    hess[0][1] = hess[1][0] = hess01(coeffs);
    hess[0][2] = hess[2][0] = hess02(coeffs);
    hess[1][2] = hess[2][1] = hess12(coeffs);
}

static double hess00(coeffs)
double   *coeffs;
{
    double    val;

    val = (4 * (a1tt * a1 * a1 * a1 + a2tt * a2 * a2 * a2 + a3tt * a3 * a3 * a3) +
	   12 * (a1t * a1t * a1 * a1 + a2t * a2t * a2 * a2 + a3t * a3t * a3 * a3)) * coeffs[0];
    val += (4 * (b1tt * b1 * b1 * b1 + b2tt * b2 * b2 * b2 + b3tt * b3 * b3 * b3) +
	    12 * (b1t * b1t * b1 * b1 + b2t * b2t * b2 * b2 + b3t * b3t * b3 * b3)) * coeffs[1];
    val += (4 * (c1tt * c1 * c1 * c1 + c2tt * c2 * c2 * c2 + c3tt * c3 * c3 * c3) +
	    12 * (c1t * c1t * c1 * c1 + c2t * c2t * c2 * c2 + c3t * c3t * c3 * c3)) * coeffs[2];
    val += (12 * (a1tt * a1 * a1 * b1 + a2tt * a2 * a2 * b2 + a3tt * a3 * a3 * b3) +
	    24 * (a1t * a1t * a1 * b1 + a2t * a2t * a2 * b2 + a3t * a3t * a3 * b3) +
	    24 * (a1t * a1 * a1 * b1t + a2t * a2 * a2 * b2t + a3t * a3 * a3 * b3t) +
	    4 * (a1 * a1 * a1 * b1tt + a2 * a2 * a2 * b2tt + a3 * a3 * a3 * b3tt)) * coeffs[3];
    val += (12 * (a1tt * a1 * b1 * b1 + a2tt * a2 * b2 * b2 + a3tt * a3 * b3 * b3) +
	    12 * (a1t * a1t * b1 * b1 + a2t * a2t * b2 * b2 + a3t * a3t * b3 * b3) +
	    48 * (a1t * a1 * b1t * b1 + a2t * a2 * b2t * b2 + a3t * a3 * b3t * b3) +
	    12 * (a1 * a1 * b1tt * b1 + a2 * a2 * b2tt * b2 + a3 * a3 * b3tt * b3) +
	    12 * (a1 * a1 * b1t * b1t + a2 * a2 * b2t * b2t + a3 * a3 * b3t * b3t)) * coeffs[4];
    val += (4 * (a1tt * b1 * b1 * b1 + a2tt * b2 * b2 * b2 + a3tt * b3 * b3 * b3) +
	    24 * (a1t * b1t * b1 * b1 + a2t * b2t * b2 * b2 + a3t * b3t * b3 * b3) +
	    12 * (a1 * b1tt * b1 * b1 + a2 * b2tt * b2 * b2 + a3 * b3tt * b3 * b3) +
	    24 * (a1 * b1t * b1t * b1 + a2 * b2t * b2t * b2 + a3 * b3t * b3t * b3)) * coeffs[5];
    val += (12 * (a1tt * a1 * a1 * c1 + a2tt * a2 * a2 * c2 + a3tt * a3 * a3 * c3) +
	    24 * (a1t * a1t * a1 * c1 + a2t * a2t * a2 * c2 + a3t * a3t * a3 * c3) +
	    24 * (a1t * a1 * a1 * c1t + a2t * a2 * a2 * c2t + a3t * a3 * a3 * c3t) +
	    4 * (a1 * a1 * a1 * c1tt + a2 * a2 * a2 * c2tt + a3 * a3 * a3 * c3tt)) * coeffs[6];
    val += (12 * (a1tt * a1 * c1 * c1 + a2tt * a2 * c2 * c2 + a3tt * a3 * c3 * c3) +
	    12 * (a1t * a1t * c1 * c1 + a2t * a2t * c2 * c2 + a3t * a3t * c3 * c3) +
	    48 * (a1t * a1 * c1t * c1 + a2t * a2 * c2t * c2 + a3t * a3 * c3t * c3) +
	    12 * (a1 * a1 * c1tt * c1 + a2 * a2 * c2tt * c2 + a3 * a3 * c3tt * c3) +
	    12 * (a1 * a1 * c1t * c1t + a2 * a2 * c2t * c2t + a3 * a3 * c3t * c3t)) * coeffs[7];
    val += (4 * (a1tt * c1 * c1 * c1 + a2tt * c2 * c2 * c2 + a3tt * c3 * c3 * c3) +
	    24 * (a1t * c1t * c1 * c1 + a2t * c2t * c2 * c2 + a3t * c3t * c3 * c3) +
	    12 * (a1 * c1tt * c1 * c1 + a2 * c2tt * c2 * c2 + a3 * c3tt * c3 * c3) +
	    24 * (a1 * c1t * c1t * c1 + a2 * c2t * c2t * c2 + a3 * c3t * c3t * c3)) * coeffs[8];
    val += (12 * (b1tt * b1 * b1 * c1 + b2tt * b2 * b2 * c2 + b3tt * b3 * b3 * c3) +
	    24 * (b1t * b1t * b1 * c1 + b2t * b2t * b2 * c2 + b3t * b3t * b3 * c3) +
	    24 * (b1t * b1 * b1 * c1t + b2t * b2 * b2 * c2t + b3t * b3 * b3 * c3t) +
	    4 * (b1 * b1 * b1 * c1tt + b2 * b2 * b2 * c2tt + b3 * b3 * b3 * c3tt)) * coeffs[9];
    val += (12 * (b1tt * b1 * c1 * c1 + b2tt * b2 * c2 * c2 + b3tt * b3 * c3 * c3) +
	    12 * (b1t * b1t * c1 * c1 + b2t * b2t * c2 * c2 + b3t * b3t * c3 * c3) +
	    48 * (b1t * b1 * c1t * c1 + b2t * b2 * c2t * c2 + b3t * b3 * c3t * c3) +
	    12 * (b1 * b1 * c1tt * c1 + b2 * b2 * c2tt * c2 + b3 * b3 * c3tt * c3) +
	    12 * (b1 * b1 * c1t * c1t + b2 * b2 * c2t * c2t + b3 * b3 * c3t * c3t)) * coeffs[10];
    val += (4 * (b1tt * c1 * c1 * c1 + b2tt * c2 * c2 * c2 + b3tt * c3 * c3 * c3) +
	    24 * (b1t * c1t * c1 * c1 + b2t * c2t * c2 * c2 + b3t * c3t * c3 * c3) +
	    12 * (b1 * c1tt * c1 * c1 + b2 * c2tt * c2 * c2 + b3 * c3tt * c3 * c3) +
	    24 * (b1 * c1t * c1t * c1 + b2 * c2t * c2t * c2 + b3 * c3t * c3t * c3)) * coeffs[11];
    val += (24 * (a1tt * a1 * b1 * c1 + a2tt * a2 * b2 * c2 + a3tt * a3 * b3 * c3) +
	    24 * (a1t * a1t * b1 * c1 + a2t * a2t * b2 * c2 + a3t * a3t * b3 * c3) +
	    48 * (a1t * a1 * b1t * c1 + a2t * a2 * b2t * c2 + a3t * a3 * b3t * c3) +
	    48 * (a1t * a1 * b1 * c1t + a2t * a2 * b2 * c2t + a3t * a3 * b3 * c3t) +
	    12 * (a1 * a1 * b1tt * c1 + a2 * a2 * b2tt * c2 + a3 * a3 * b3tt * c3) +
	    24 * (a1 * a1 * b1t * c1t + a2 * a2 * b2t * c2t + a3 * a3 * b3t * c3t) +
	    12 * (a1 * a1 * b1 * c1tt + a2 * a2 * b2 * c2tt + a3 * a3 * b3 * c3tt)) * coeffs[12];
    val += (12 * (a1tt * b1 * b1 * c1 + a2tt * b2 * b2 * c2 + a3tt * b3 * b3 * c3) +
	    48 * (a1t * b1t * b1 * c1 + a2t * b2t * b2 * c2 + a3t * b3t * b3 * c3) +
	    24 * (a1t * b1 * b1 * c1t + a2t * b2 * b2 * c2t + a3t * b3 * b3 * c3t) +
	    24 * (a1 * b1tt * b1 * c1 + a2 * b2tt * b2 * c2 + a3 * b3tt * b3 * c3) +
	    24 * (a1 * b1t * b1t * c1 + a2 * b2t * b2t * c2 + a3 * b3t * b3t * c3) +
	    48 * (a1 * b1t * b1 * c1t + a2 * b2t * b2 * c2t + a3 * b3t * b3 * c3t) +
	    12 * (a1 * b1 * b1 * c1tt + a2 * b2 * b2 * c2tt + a3 * b3 * b3 * c3tt)) * coeffs[13];
    val += (12 * (a1tt * b1 * c1 * c1 + a2tt * b2 * c2 * c2 + a3tt * b3 * c3 * c3) +
	    24 * (a1t * b1t * c1 * c1 + a2t * b2t * c2 * c2 + a3t * b3t * c3 * c3) +
	    48 * (a1t * b1 * c1t * c1 + a2t * b2 * c2t * c2 + a3t * b3 * c3t * c3) +
	    12 * (a1 * b1tt * c1 * c1 + a2 * b2tt * c2 * c2 + a3 * b3tt * c3 * c3) +
	    48 * (a1 * b1t * c1t * c1 + a2 * b2t * c2t * c2 + a3 * b3t * c3t * c3) +
	    24 * (a1 * b1 * c1tt * c1 + a2 * b2 * c2tt * c2 + a3 * b3 * c3tt * c3) +
	    24 * (a1 * b1 * c1t * c1t + a2 * b2 * c2t * c2t + a3 * b3 * c3t * c3t)) * coeffs[14];
    return (val);
}

static double hess11(coeffs)
double   *coeffs;
{
    double    val;

    val = (4 * (a1pp * a1 * a1 * a1 + a2pp * a2 * a2 * a2 + a3pp * a3 * a3 * a3) +
	   12 * (a1p * a1p * a1 * a1 + a2p * a2p * a2 * a2 + a3p * a3p * a3 * a3)) * coeffs[0];
    val += (4 * (b1pp * b1 * b1 * b1 + b2pp * b2 * b2 * b2 + b3pp * b3 * b3 * b3) +
	    12 * (b1p * b1p * b1 * b1 + b2p * b2p * b2 * b2 + b3p * b3p * b3 * b3)) * coeffs[1];
    val += (4 * (c1pp * c1 * c1 * c1 + c2pp * c2 * c2 * c2 + c3pp * c3 * c3 * c3) +
	    12 * (c1p * c1p * c1 * c1 + c2p * c2p * c2 * c2 + c3p * c3p * c3 * c3)) * coeffs[2];
    val += (12 * (a1pp * a1 * a1 * b1 + a2pp * a2 * a2 * b2 + a3pp * a3 * a3 * b3) +
	    24 * (a1p * a1p * a1 * b1 + a2p * a2p * a2 * b2 + a3p * a3p * a3 * b3) +
	    24 * (a1p * a1 * a1 * b1p + a2p * a2 * a2 * b2p + a3p * a3 * a3 * b3p) +
	    4 * (a1 * a1 * a1 * b1pp + a2 * a2 * a2 * b2pp + a3 * a3 * a3 * b3pp)) * coeffs[3];
    val += (12 * (a1pp * a1 * b1 * b1 + a2pp * a2 * b2 * b2 + a3pp * a3 * b3 * b3) +
	    12 * (a1p * a1p * b1 * b1 + a2p * a2p * b2 * b2 + a3p * a3p * b3 * b3) +
	    48 * (a1p * a1 * b1p * b1 + a2p * a2 * b2p * b2 + a3p * a3 * b3p * b3) +
	    12 * (a1 * a1 * b1pp * b1 + a2 * a2 * b2pp * b2 + a3 * a3 * b3pp * b3) +
	    12 * (a1 * a1 * b1p * b1p + a2 * a2 * b2p * b2p + a3 * a3 * b3p * b3p)) * coeffs[4];
    val += (4 * (a1pp * b1 * b1 * b1 + a2pp * b2 * b2 * b2 + a3pp * b3 * b3 * b3) +
	    24 * (a1p * b1p * b1 * b1 + a2p * b2p * b2 * b2 + a3p * b3p * b3 * b3) +
	    12 * (a1 * b1pp * b1 * b1 + a2 * b2pp * b2 * b2 + a3 * b3pp * b3 * b3) +
	    24 * (a1 * b1p * b1p * b1 + a2 * b2p * b2p * b2 + a3 * b3p * b3p * b3)) * coeffs[5];
    val += (12 * (a1pp * a1 * a1 * c1 + a2pp * a2 * a2 * c2 + a3pp * a3 * a3 * c3) +
	    24 * (a1p * a1p * a1 * c1 + a2p * a2p * a2 * c2 + a3p * a3p * a3 * c3) +
	    24 * (a1p * a1 * a1 * c1p + a2p * a2 * a2 * c2p + a3p * a3 * a3 * c3p) +
	    4 * (a1 * a1 * a1 * c1pp + a2 * a2 * a2 * c2pp + a3 * a3 * a3 * c3pp)) * coeffs[6];
    val += (12 * (a1pp * a1 * c1 * c1 + a2pp * a2 * c2 * c2 + a3pp * a3 * c3 * c3) +
	    12 * (a1p * a1p * c1 * c1 + a2p * a2p * c2 * c2 + a3p * a3p * c3 * c3) +
	    48 * (a1p * a1 * c1p * c1 + a2p * a2 * c2p * c2 + a3p * a3 * c3p * c3) +
	    12 * (a1 * a1 * c1pp * c1 + a2 * a2 * c2pp * c2 + a3 * a3 * c3pp * c3) +
	    12 * (a1 * a1 * c1p * c1p + a2 * a2 * c2p * c2p + a3 * a3 * c3p * c3p)) * coeffs[7];
    val += (4 * (a1pp * c1 * c1 * c1 + a2pp * c2 * c2 * c2 + a3pp * c3 * c3 * c3) +
	    24 * (a1p * c1p * c1 * c1 + a2p * c2p * c2 * c2 + a3p * c3p * c3 * c3) +
	    12 * (a1 * c1pp * c1 * c1 + a2 * c2pp * c2 * c2 + a3 * c3pp * c3 * c3) +
	    24 * (a1 * c1p * c1p * c1 + a2 * c2p * c2p * c2 + a3 * c3p * c3p * c3)) * coeffs[8];
    val += (12 * (b1pp * b1 * b1 * c1 + b2pp * b2 * b2 * c2 + b3pp * b3 * b3 * c3) +
	    24 * (b1p * b1p * b1 * c1 + b2p * b2p * b2 * c2 + b3p * b3p * b3 * c3) +
	    24 * (b1p * b1 * b1 * c1p + b2p * b2 * b2 * c2p + b3p * b3 * b3 * c3p) +
	    4 * (b1 * b1 * b1 * c1pp + b2 * b2 * b2 * c2pp + b3 * b3 * b3 * c3pp)) * coeffs[9];
    val += (12 * (b1pp * b1 * c1 * c1 + b2pp * b2 * c2 * c2 + b3pp * b3 * c3 * c3) +
	    12 * (b1p * b1p * c1 * c1 + b2p * b2p * c2 * c2 + b3p * b3p * c3 * c3) +
	    48 * (b1p * b1 * c1p * c1 + b2p * b2 * c2p * c2 + b3p * b3 * c3p * c3) +
	    12 * (b1 * b1 * c1pp * c1 + b2 * b2 * c2pp * c2 + b3 * b3 * c3pp * c3) +
	    12 * (b1 * b1 * c1p * c1p + b2 * b2 * c2p * c2p + b3 * b3 * c3p * c3p)) * coeffs[10];
    val += (4 * (b1pp * c1 * c1 * c1 + b2pp * c2 * c2 * c2 + b3pp * c3 * c3 * c3) +
	    24 * (b1p * c1p * c1 * c1 + b2p * c2p * c2 * c2 + b3p * c3p * c3 * c3) +
	    12 * (b1 * c1pp * c1 * c1 + b2 * c2pp * c2 * c2 + b3 * c3pp * c3 * c3) +
	    24 * (b1 * c1p * c1p * c1 + b2 * c2p * c2p * c2 + b3 * c3p * c3p * c3)) * coeffs[11];
    val += (24 * (a1pp * a1 * b1 * c1 + a2pp * a2 * b2 * c2 + a3pp * a3 * b3 * c3) +
	    24 * (a1p * a1p * b1 * c1 + a2p * a2p * b2 * c2 + a3p * a3p * b3 * c3) +
	    48 * (a1p * a1 * b1p * c1 + a2p * a2 * b2p * c2 + a3p * a3 * b3p * c3) +
	    48 * (a1p * a1 * b1 * c1p + a2p * a2 * b2 * c2p + a3p * a3 * b3 * c3p) +
	    12 * (a1 * a1 * b1pp * c1 + a2 * a2 * b2pp * c2 + a3 * a3 * b3pp * c3) +
	    24 * (a1 * a1 * b1p * c1p + a2 * a2 * b2p * c2p + a3 * a3 * b3p * c3p) +
	    12 * (a1 * a1 * b1 * c1pp + a2 * a2 * b2 * c2pp + a3 * a3 * b3 * c3pp)) * coeffs[12];
    val += (12 * (a1pp * b1 * b1 * c1 + a2pp * b2 * b2 * c2 + a3pp * b3 * b3 * c3) +
	    48 * (a1p * b1p * b1 * c1 + a2p * b2p * b2 * c2 + a3p * b3p * b3 * c3) +
	    24 * (a1p * b1 * b1 * c1p + a2p * b2 * b2 * c2p + a3p * b3 * b3 * c3p) +
	    24 * (a1 * b1pp * b1 * c1 + a2 * b2pp * b2 * c2 + a3 * b3pp * b3 * c3) +
	    24 * (a1 * b1p * b1p * c1 + a2 * b2p * b2p * c2 + a3 * b3p * b3p * c3) +
	    48 * (a1 * b1p * b1 * c1p + a2 * b2p * b2 * c2p + a3 * b3p * b3 * c3p) +
	    12 * (a1 * b1 * b1 * c1pp + a2 * b2 * b2 * c2pp + a3 * b3 * b3 * c3pp)) * coeffs[13];
    val += (12 * (a1pp * b1 * c1 * c1 + a2pp * b2 * c2 * c2 + a3pp * b3 * c3 * c3) +
	    24 * (a1p * b1p * c1 * c1 + a2p * b2p * c2 * c2 + a3p * b3p * c3 * c3) +
	    48 * (a1p * b1 * c1p * c1 + a2p * b2 * c2p * c2 + a3p * b3 * c3p * c3) +
	    12 * (a1 * b1pp * c1 * c1 + a2 * b2pp * c2 * c2 + a3 * b3pp * c3 * c3) +
	    48 * (a1 * b1p * c1p * c1 + a2 * b2p * c2p * c2 + a3 * b3p * c3p * c3) +
	    24 * (a1 * b1 * c1pp * c1 + a2 * b2 * c2pp * c2 + a3 * b3 * c3pp * c3) +
	    24 * (a1 * b1 * c1p * c1p + a2 * b2 * c2p * c2p + a3 * b3 * c3p * c3p)) * coeffs[14];
    return (val);
}

static double hess22(coeffs)
double   *coeffs;
{
    double    val;

    val = (4 * (a1gg * a1 * a1 * a1 + a2gg * a2 * a2 * a2 + a3gg * a3 * a3 * a3) +
	   12 * (a1g * a1g * a1 * a1 + a2g * a2g * a2 * a2 + a3g * a3g * a3 * a3)) * coeffs[0];
    val += (4 * (b1gg * b1 * b1 * b1 + b2gg * b2 * b2 * b2 + b3gg * b3 * b3 * b3) +
	    12 * (b1g * b1g * b1 * b1 + b2g * b2g * b2 * b2 + b3g * b3g * b3 * b3)) * coeffs[1];
    val += (4 * (c1gg * c1 * c1 * c1 + c2gg * c2 * c2 * c2 + c3gg * c3 * c3 * c3) +
	    12 * (c1g * c1g * c1 * c1 + c2g * c2g * c2 * c2 + c3g * c3g * c3 * c3)) * coeffs[2];
    val += (12 * (a1gg * a1 * a1 * b1 + a2gg * a2 * a2 * b2 + a3gg * a3 * a3 * b3) +
	    24 * (a1g * a1g * a1 * b1 + a2g * a2g * a2 * b2 + a3g * a3g * a3 * b3) +
	    24 * (a1g * a1 * a1 * b1g + a2g * a2 * a2 * b2g + a3g * a3 * a3 * b3g) +
	    4 * (a1 * a1 * a1 * b1gg + a2 * a2 * a2 * b2gg + a3 * a3 * a3 * b3gg)) * coeffs[3];
    val += (12 * (a1gg * a1 * b1 * b1 + a2gg * a2 * b2 * b2 + a3gg * a3 * b3 * b3) +
	    12 * (a1g * a1g * b1 * b1 + a2g * a2g * b2 * b2 + a3g * a3g * b3 * b3) +
	    48 * (a1g * a1 * b1g * b1 + a2g * a2 * b2g * b2 + a3g * a3 * b3g * b3) +
	    12 * (a1 * a1 * b1gg * b1 + a2 * a2 * b2gg * b2 + a3 * a3 * b3gg * b3) +
	    12 * (a1 * a1 * b1g * b1g + a2 * a2 * b2g * b2g + a3 * a3 * b3g * b3g)) * coeffs[4];
    val += (4 * (a1gg * b1 * b1 * b1 + a2gg * b2 * b2 * b2 + a3gg * b3 * b3 * b3) +
	    24 * (a1g * b1g * b1 * b1 + a2g * b2g * b2 * b2 + a3g * b3g * b3 * b3) +
	    12 * (a1 * b1gg * b1 * b1 + a2 * b2gg * b2 * b2 + a3 * b3gg * b3 * b3) +
	    24 * (a1 * b1g * b1g * b1 + a2 * b2g * b2g * b2 + a3 * b3g * b3g * b3)) * coeffs[5];
    val += (12 * (a1gg * a1 * a1 * c1 + a2gg * a2 * a2 * c2 + a3gg * a3 * a3 * c3) +
	    24 * (a1g * a1g * a1 * c1 + a2g * a2g * a2 * c2 + a3g * a3g * a3 * c3) +
	    24 * (a1g * a1 * a1 * c1g + a2g * a2 * a2 * c2g + a3g * a3 * a3 * c3g) +
	    4 * (a1 * a1 * a1 * c1gg + a2 * a2 * a2 * c2gg + a3 * a3 * a3 * c3gg)) * coeffs[6];
    val += (12 * (a1gg * a1 * c1 * c1 + a2gg * a2 * c2 * c2 + a3gg * a3 * c3 * c3) +
	    12 * (a1g * a1g * c1 * c1 + a2g * a2g * c2 * c2 + a3g * a3g * c3 * c3) +
	    48 * (a1g * a1 * c1g * c1 + a2g * a2 * c2g * c2 + a3g * a3 * c3g * c3) +
	    12 * (a1 * a1 * c1gg * c1 + a2 * a2 * c2gg * c2 + a3 * a3 * c3gg * c3) +
	    12 * (a1 * a1 * c1g * c1g + a2 * a2 * c2g * c2g + a3 * a3 * c3g * c3g)) * coeffs[7];
    val += (4 * (a1gg * c1 * c1 * c1 + a2gg * c2 * c2 * c2 + a3gg * c3 * c3 * c3) +
	    24 * (a1g * c1g * c1 * c1 + a2g * c2g * c2 * c2 + a3g * c3g * c3 * c3) +
	    12 * (a1 * c1gg * c1 * c1 + a2 * c2gg * c2 * c2 + a3 * c3gg * c3 * c3) +
	    24 * (a1 * c1g * c1g * c1 + a2 * c2g * c2g * c2 + a3 * c3g * c3g * c3)) * coeffs[8];
    val += (12 * (b1gg * b1 * b1 * c1 + b2gg * b2 * b2 * c2 + b3gg * b3 * b3 * c3) +
	    24 * (b1g * b1g * b1 * c1 + b2g * b2g * b2 * c2 + b3g * b3g * b3 * c3) +
	    24 * (b1g * b1 * b1 * c1g + b2g * b2 * b2 * c2g + b3g * b3 * b3 * c3g) +
	    4 * (b1 * b1 * b1 * c1gg + b2 * b2 * b2 * c2gg + b3 * b3 * b3 * c3gg)) * coeffs[9];
    val += (12 * (b1gg * b1 * c1 * c1 + b2gg * b2 * c2 * c2 + b3gg * b3 * c3 * c3) +
	    12 * (b1g * b1g * c1 * c1 + b2g * b2g * c2 * c2 + b3g * b3g * c3 * c3) +
	    48 * (b1g * b1 * c1g * c1 + b2g * b2 * c2g * c2 + b3g * b3 * c3g * c3) +
	    12 * (b1 * b1 * c1gg * c1 + b2 * b2 * c2gg * c2 + b3 * b3 * c3gg * c3) +
	    12 * (b1 * b1 * c1g * c1g + b2 * b2 * c2g * c2g + b3 * b3 * c3g * c3g)) * coeffs[10];
    val += (4 * (b1gg * c1 * c1 * c1 + b2gg * c2 * c2 * c2 + b3gg * c3 * c3 * c3) +
	    24 * (b1g * c1g * c1 * c1 + b2g * c2g * c2 * c2 + b3g * c3g * c3 * c3) +
	    12 * (b1 * c1gg * c1 * c1 + b2 * c2gg * c2 * c2 + b3 * c3gg * c3 * c3) +
	    24 * (b1 * c1g * c1g * c1 + b2 * c2g * c2g * c2 + b3 * c3g * c3g * c3)) * coeffs[11];
    val += (24 * (a1gg * a1 * b1 * c1 + a2gg * a2 * b2 * c2 + a3gg * a3 * b3 * c3) +
	    24 * (a1g * a1g * b1 * c1 + a2g * a2g * b2 * c2 + a3g * a3g * b3 * c3) +
	    48 * (a1g * a1 * b1g * c1 + a2g * a2 * b2g * c2 + a3g * a3 * b3g * c3) +
	    48 * (a1g * a1 * b1 * c1g + a2g * a2 * b2 * c2g + a3g * a3 * b3 * c3g) +
	    12 * (a1 * a1 * b1gg * c1 + a2 * a2 * b2gg * c2 + a3 * a3 * b3gg * c3) +
	    24 * (a1 * a1 * b1g * c1g + a2 * a2 * b2g * c2g + a3 * a3 * b3g * c3g) +
	    12 * (a1 * a1 * b1 * c1gg + a2 * a2 * b2 * c2gg + a3 * a3 * b3 * c3gg)) * coeffs[12];
    val += (12 * (a1gg * b1 * b1 * c1 + a2gg * b2 * b2 * c2 + a3gg * b3 * b3 * c3) +
	    48 * (a1g * b1g * b1 * c1 + a2g * b2g * b2 * c2 + a3g * b3g * b3 * c3) +
	    24 * (a1g * b1 * b1 * c1g + a2g * b2 * b2 * c2g + a3g * b3 * b3 * c3g) +
	    24 * (a1 * b1gg * b1 * c1 + a2 * b2gg * b2 * c2 + a3 * b3gg * b3 * c3) +
	    24 * (a1 * b1g * b1g * c1 + a2 * b2g * b2g * c2 + a3 * b3g * b3g * c3) +
	    48 * (a1 * b1g * b1 * c1g + a2 * b2g * b2 * c2g + a3 * b3g * b3 * c3g) +
	    12 * (a1 * b1 * b1 * c1gg + a2 * b2 * b2 * c2gg + a3 * b3 * b3 * c3gg)) * coeffs[13];
    val += (12 * (a1gg * b1 * c1 * c1 + a2gg * b2 * c2 * c2 + a3gg * b3 * c3 * c3) +
	    24 * (a1g * b1g * c1 * c1 + a2g * b2g * c2 * c2 + a3g * b3g * c3 * c3) +
	    48 * (a1g * b1 * c1g * c1 + a2g * b2 * c2g * c2 + a3g * b3 * c3g * c3) +
	    12 * (a1 * b1gg * c1 * c1 + a2 * b2gg * c2 * c2 + a3 * b3gg * c3 * c3) +
	    48 * (a1 * b1g * c1g * c1 + a2 * b2g * c2g * c2 + a3 * b3g * c3g * c3) +
	    24 * (a1 * b1 * c1gg * c1 + a2 * b2 * c2gg * c2 + a3 * b3 * c3gg * c3) +
	    24 * (a1 * b1 * c1g * c1g + a2 * b2 * c2g * c2g + a3 * b3 * c3g * c3g)) * coeffs[14];
    return (val);
}

static double hess01(coeffs)
double   *coeffs;
{
    double    val;

    val = (4 * (a1tp * a1 * a1 * a1 + a2tp * a2 * a2 * a2 + a3tp * a3 * a3 * a3) +
	   12 * (a1t * a1p * a1 * a1 + a2t * a2p * a2 * a2 + a3t * a3p * a3 * a3)) * coeffs[0];
    val += (4 * (b1tp * b1 * b1 * b1 + b2tp * b2 * b2 * b2 + b3tp * b3 * b3 * b3) +
	    12 * (b1t * b1p * b1 * b1 + b2t * b2p * b2 * b2 + b3t * b3p * b3 * b3)) * coeffs[1];
    val += (4 * (c1tp * c1 * c1 * c1 + c2tp * c2 * c2 * c2 + c3tp * c3 * c3 * c3) +
	    12 * (c1t * c1p * c1 * c1 + c2t * c2p * c2 * c2 + c3t * c3p * c3 * c3)) * coeffs[2];
    val += (12 * (a1tp * a1 * a1 * b1 + a2tp * a2 * a2 * b2 + a3tp * a3 * a3 * b3) +
	    24 * (a1t * a1p * a1 * b1 + a2t * a2p * a2 * b2 + a3t * a3p * a3 * b3) +
	    12 * (a1t * a1 * a1 * b1p + a2t * a2 * a2 * b2p + a3t * a3 * a3 * b3p) +
	    12 * (a1p * a1 * a1 * b1t + a2p * a2 * a2 * b2t + a3p * a3 * a3 * b3t) +
	    4 * (a1 * a1 * a1 * b1tp + a2 * a2 * a2 * b2tp + a3 * a3 * a3 * b3tp)) * coeffs[3];
    val += (12 * (a1tp * a1 * b1 * b1 + a2tp * a2 * b2 * b2 + a3tp * a3 * b3 * b3) +
	    12 * (a1t * a1p * b1 * b1 + a2t * a2p * b2 * b2 + a3t * a3p * b3 * b3) +
	    24 * (a1t * a1 * b1p * b1 + a2t * a2 * b2p * b2 + a3t * a3 * b3p * b3) +
	    24 * (a1p * a1 * b1t * b1 + a2p * a2 * b2t * b2 + a3p * a3 * b3t * b3) +
	    12 * (a1 * a1 * b1tp * b1 + a2 * a2 * b2tp * b2 + a3 * a3 * b3tp * b3) +
	    12 * (a1 * a1 * b1t * b1p + a2 * a2 * b2t * b2p + a3 * a3 * b3t * b3p)) * coeffs[4];
    val += (4 * (a1tp * b1 * b1 * b1 + a2tp * b2 * b2 * b2 + a3tp * b3 * b3 * b3) +
	    12 * (a1t * b1p * b1 * b1 + a2t * b2p * b2 * b2 + a3t * b3p * b3 * b3) +
	    12 * (a1p * b1t * b1 * b1 + a2p * b2t * b2 * b2 + a3p * b3t * b3 * b3) +
	    12 * (a1 * b1tp * b1 * b1 + a2 * b2tp * b2 * b2 + a3 * b3tp * b3 * b3) +
	    24 * (a1 * b1t * b1p * b1 + a2 * b2t * b2p * b2 + a3 * b3t * b3p * b3)) * coeffs[5];
    val += (12 * (a1tp * a1 * a1 * c1 + a2tp * a2 * a2 * c2 + a3tp * a3 * a3 * c3) +
	    24 * (a1t * a1p * a1 * c1 + a2t * a2p * a2 * c2 + a3t * a3p * a3 * c3) +
	    12 * (a1t * a1 * a1 * c1p + a2t * a2 * a2 * c2p + a3t * a3 * a3 * c3p) +
	    12 * (a1p * a1 * a1 * c1t + a2p * a2 * a2 * c2t + a3p * a3 * a3 * c3t) +
	    4 * (a1 * a1 * a1 * c1tp + a2 * a2 * a2 * c2tp + a3 * a3 * a3 * c3tp)) * coeffs[6];
    val += (12 * (a1tp * a1 * c1 * c1 + a2tp * a2 * c2 * c2 + a3tp * a3 * c3 * c3) +
	    12 * (a1t * a1p * c1 * c1 + a2t * a2p * c2 * c2 + a3t * a3p * c3 * c3) +
	    24 * (a1t * a1 * c1p * c1 + a2t * a2 * c2p * c2 + a3t * a3 * c3p * c3) +
	    24 * (a1p * a1 * c1t * c1 + a2p * a2 * c2t * c2 + a3p * a3 * c3t * c3) +
	    12 * (a1 * a1 * c1tp * c1 + a2 * a2 * c2tp * c2 + a3 * a3 * c3tp * c3) +
	    12 * (a1 * a1 * c1t * c1p + a2 * a2 * c2t * c2p + a3 * a3 * c3t * c3p)) * coeffs[7];
    val += (4 * (a1tp * c1 * c1 * c1 + a2tp * c2 * c2 * c2 + a3tp * c3 * c3 * c3) +
	    12 * (a1t * c1p * c1 * c1 + a2t * c2p * c2 * c2 + a3t * c3p * c3 * c3) +
	    12 * (a1p * c1t * c1 * c1 + a2p * c2t * c2 * c2 + a3p * c3t * c3 * c3) +
	    12 * (a1 * c1tp * c1 * c1 + a2 * c2tp * c2 * c2 + a3 * c3tp * c3 * c3) +
	    24 * (a1 * c1t * c1p * c1 + a2 * c2t * c2p * c2 + a3 * c3t * c3p * c3)) * coeffs[8];
    val += (12 * (b1tp * b1 * b1 * c1 + b2tp * b2 * b2 * c2 + b3tp * b3 * b3 * c3) +
	    24 * (b1t * b1p * b1 * c1 + b2t * b2p * b2 * c2 + b3t * b3p * b3 * c3) +
	    12 * (b1t * b1 * b1 * c1p + b2t * b2 * b2 * c2p + b3t * b3 * b3 * c3p) +
	    12 * (b1p * b1 * b1 * c1t + b2p * b2 * b2 * c2t + b3p * b3 * b3 * c3t) +
	    4 * (b1 * b1 * b1 * c1tp + b2 * b2 * b2 * c2tp + b3 * b3 * b3 * c3tp)) * coeffs[9];
    val += (12 * (b1tp * b1 * c1 * c1 + b2tp * b2 * c2 * c2 + b3tp * b3 * c3 * c3) +
	    12 * (b1t * b1p * c1 * c1 + b2t * b2p * c2 * c2 + b3t * b3p * c3 * c3) +
	    24 * (b1t * b1 * c1p * c1 + b2t * b2 * c2p * c2 + b3t * b3 * c3p * c3) +
	    24 * (b1p * b1 * c1t * c1 + b2p * b2 * c2t * c2 + b3p * b3 * c3t * c3) +
	    12 * (b1 * b1 * c1tp * c1 + b2 * b2 * c2tp * c2 + b3 * b3 * c3tp * c3) +
	    12 * (b1 * b1 * c1t * c1p + b2 * b2 * c2t * c2p + b3 * b3 * c3t * c3p)) * coeffs[10];
    val += (4 * (b1tp * c1 * c1 * c1 + b2tp * c2 * c2 * c2 + b3tp * c3 * c3 * c3) +
	    12 * (b1t * c1p * c1 * c1 + b2t * c2p * c2 * c2 + b3t * c3p * c3 * c3) +
	    12 * (b1p * c1t * c1 * c1 + b2p * c2t * c2 * c2 + b3p * c3t * c3 * c3) +
	    12 * (b1 * c1tp * c1 * c1 + b2 * c2tp * c2 * c2 + b3 * c3tp * c3 * c3) +
	    24 * (b1 * c1t * c1p * c1 + b2 * c2t * c2p * c2 + b3 * c3t * c3p * c3)) * coeffs[11];
    val += (24 * (a1tp * a1 * b1 * c1 + a2tp * a2 * b2 * c2 + a3tp * a3 * b3 * c3) +
	    24 * (a1t * a1p * b1 * c1 + a2t * a2p * b2 * c2 + a3t * a3p * b3 * c3) +
	    24 * (a1t * a1 * b1p * c1 + a2t * a2 * b2p * c2 + a3t * a3 * b3p * c3) +
	    24 * (a1p * a1 * b1t * c1 + a2p * a2 * b2t * c2 + a3p * a3 * b3t * c3) +
	    24 * (a1t * a1 * b1 * c1p + a2t * a2 * b2 * c2p + a3t * a3 * b3 * c3p) +
	    24 * (a1p * a1 * b1 * c1t + a2p * a2 * b2 * c2t + a3p * a3 * b3 * c3t) +
	    12 * (a1 * a1 * b1tp * c1 + a2 * a2 * b2tp * c2 + a3 * a3 * b3tp * c3) +
	    12 * (a1 * a1 * b1t * c1p + a2 * a2 * b2t * c2p + a3 * a3 * b3t * c3p) +
	    12 * (a1 * a1 * b1p * c1t + a2 * a2 * b2p * c2t + a3 * a3 * b3p * c3t) +
	    12 * (a1 * a1 * b1 * c1tp + a2 * a2 * b2 * c2tp + a3 * a3 * b3 * c3tp)) * coeffs[12];
    val += (12 * (a1tp * b1 * b1 * c1 + a2tp * b2 * b2 * c2 + a3tp * b3 * b3 * c3) +
	    24 * (a1t * b1p * b1 * c1 + a2t * b2p * b2 * c2 + a3t * b3p * b3 * c3) +
	    24 * (a1p * b1t * b1 * c1 + a2p * b2t * b2 * c2 + a3p * b3t * b3 * c3) +
	    12 * (a1t * b1 * b1 * c1p + a2t * b2 * b2 * c2p + a3t * b3 * b3 * c3p) +
	    12 * (a1p * b1 * b1 * c1t + a2p * b2 * b2 * c2t + a3p * b3 * b3 * c3t) +
	    24 * (a1 * b1tp * b1 * c1 + a2 * b2tp * b2 * c2 + a3 * b3tp * b3 * c3) +
	    24 * (a1 * b1t * b1p * c1 + a2 * b2t * b2p * c2 + a3 * b3t * b3p * c3) +
	    24 * (a1 * b1t * b1 * c1p + a2 * b2t * b2 * c2p + a3 * b3t * b3 * c3p) +
	    24 * (a1 * b1p * b1 * c1t + a2 * b2p * b2 * c2t + a3 * b3p * b3 * c3t) +
	    12 * (a1 * b1 * b1 * c1tp + a2 * b2 * b2 * c2tp + a3 * b3 * b3 * c3tp)) * coeffs[13];
    val += (12 * (a1tp * b1 * c1 * c1 + a2tp * b2 * c2 * c2 + a3tp * b3 * c3 * c3) +
	    12 * (a1t * b1p * c1 * c1 + a2t * b2p * c2 * c2 + a3t * b3p * c3 * c3) +
	    12 * (a1p * b1t * c1 * c1 + a2p * b2t * c2 * c2 + a3p * b3t * c3 * c3) +
	    24 * (a1t * b1 * c1p * c1 + a2t * b2 * c2p * c2 + a3t * b3 * c3p * c3) +
	    24 * (a1p * b1 * c1t * c1 + a2p * b2 * c2t * c2 + a3p * b3 * c3t * c3) +
	    12 * (a1 * b1tp * c1 * c1 + a2 * b2tp * c2 * c2 + a3 * b3tp * c3 * c3) +
	    24 * (a1 * b1t * c1p * c1 + a2 * b2t * c2p * c2 + a3 * b3t * c3p * c3) +
	    24 * (a1 * b1p * c1t * c1 + a2 * b2p * c2t * c2 + a3 * b3p * c3t * c3) +
	    24 * (a1 * b1 * c1tp * c1 + a2 * b2 * c2tp * c2 + a3 * b3 * c3tp * c3) +
	    24 * (a1 * b1 * c1t * c1p + a2 * b2 * c2t * c2p + a3 * b3 * c3t * c3p)) * coeffs[14];
    return (val);
}

static double hess02(coeffs)
double   *coeffs;
{
    double    val;

    val = (4 * (a1tg * a1 * a1 * a1 + a2tg * a2 * a2 * a2 + a3tg * a3 * a3 * a3) +
	   12 * (a1t * a1g * a1 * a1 + a2t * a2g * a2 * a2 + a3t * a3g * a3 * a3)) * coeffs[0];
    val += (4 * (b1tg * b1 * b1 * b1 + b2tg * b2 * b2 * b2 + b3tg * b3 * b3 * b3) +
	    12 * (b1t * b1g * b1 * b1 + b2t * b2g * b2 * b2 + b3t * b3g * b3 * b3)) * coeffs[1];
    val += (4 * (c1tg * c1 * c1 * c1 + c2tg * c2 * c2 * c2 + c3tg * c3 * c3 * c3) +
	    12 * (c1t * c1g * c1 * c1 + c2t * c2g * c2 * c2 + c3t * c3g * c3 * c3)) * coeffs[2];
    val += (12 * (a1tg * a1 * a1 * b1 + a2tg * a2 * a2 * b2 + a3tg * a3 * a3 * b3) +
	    24 * (a1t * a1g * a1 * b1 + a2t * a2g * a2 * b2 + a3t * a3g * a3 * b3) +
	    12 * (a1t * a1 * a1 * b1g + a2t * a2 * a2 * b2g + a3t * a3 * a3 * b3g) +
	    12 * (a1g * a1 * a1 * b1t + a2g * a2 * a2 * b2t + a3g * a3 * a3 * b3t) +
	    4 * (a1 * a1 * a1 * b1tg + a2 * a2 * a2 * b2tg + a3 * a3 * a3 * b3tg)) * coeffs[3];
    val += (12 * (a1tg * a1 * b1 * b1 + a2tg * a2 * b2 * b2 + a3tg * a3 * b3 * b3) +
	    12 * (a1t * a1g * b1 * b1 + a2t * a2g * b2 * b2 + a3t * a3g * b3 * b3) +
	    24 * (a1t * a1 * b1g * b1 + a2t * a2 * b2g * b2 + a3t * a3 * b3g * b3) +
	    24 * (a1g * a1 * b1t * b1 + a2g * a2 * b2t * b2 + a3g * a3 * b3t * b3) +
	    12 * (a1 * a1 * b1tg * b1 + a2 * a2 * b2tg * b2 + a3 * a3 * b3tg * b3) +
	    12 * (a1 * a1 * b1t * b1g + a2 * a2 * b2t * b2g + a3 * a3 * b3t * b3g)) * coeffs[4];
    val += (4 * (a1tg * b1 * b1 * b1 + a2tg * b2 * b2 * b2 + a3tg * b3 * b3 * b3) +
	    12 * (a1t * b1g * b1 * b1 + a2t * b2g * b2 * b2 + a3t * b3g * b3 * b3) +
	    12 * (a1g * b1t * b1 * b1 + a2g * b2t * b2 * b2 + a3g * b3t * b3 * b3) +
	    12 * (a1 * b1tg * b1 * b1 + a2 * b2tg * b2 * b2 + a3 * b3tg * b3 * b3) +
	    24 * (a1 * b1t * b1g * b1 + a2 * b2t * b2g * b2 + a3 * b3t * b3g * b3)) * coeffs[5];
    val += (12 * (a1tg * a1 * a1 * c1 + a2tg * a2 * a2 * c2 + a3tg * a3 * a3 * c3) +
	    24 * (a1t * a1g * a1 * c1 + a2t * a2g * a2 * c2 + a3t * a3g * a3 * c3) +
	    12 * (a1t * a1 * a1 * c1g + a2t * a2 * a2 * c2g + a3t * a3 * a3 * c3g) +
	    12 * (a1g * a1 * a1 * c1t + a2g * a2 * a2 * c2t + a3g * a3 * a3 * c3t) +
	    4 * (a1 * a1 * a1 * c1tg + a2 * a2 * a2 * c2tg + a3 * a3 * a3 * c3tg)) * coeffs[6];
    val += (12 * (a1tg * a1 * c1 * c1 + a2tg * a2 * c2 * c2 + a3tg * a3 * c3 * c3) +
	    12 * (a1t * a1g * c1 * c1 + a2t * a2g * c2 * c2 + a3t * a3g * c3 * c3) +
	    24 * (a1t * a1 * c1g * c1 + a2t * a2 * c2g * c2 + a3t * a3 * c3g * c3) +
	    24 * (a1g * a1 * c1t * c1 + a2g * a2 * c2t * c2 + a3g * a3 * c3t * c3) +
	    12 * (a1 * a1 * c1tg * c1 + a2 * a2 * c2tg * c2 + a3 * a3 * c3tg * c3) +
	    12 * (a1 * a1 * c1t * c1g + a2 * a2 * c2t * c2g + a3 * a3 * c3t * c3g)) * coeffs[7];
    val += (4 * (a1tg * c1 * c1 * c1 + a2tg * c2 * c2 * c2 + a3tg * c3 * c3 * c3) +
	    12 * (a1t * c1g * c1 * c1 + a2t * c2g * c2 * c2 + a3t * c3g * c3 * c3) +
	    12 * (a1g * c1t * c1 * c1 + a2g * c2t * c2 * c2 + a3g * c3t * c3 * c3) +
	    12 * (a1 * c1tg * c1 * c1 + a2 * c2tg * c2 * c2 + a3 * c3tg * c3 * c3) +
	    24 * (a1 * c1t * c1g * c1 + a2 * c2t * c2g * c2 + a3 * c3t * c3g * c3)) * coeffs[8];
    val += (12 * (b1tg * b1 * b1 * c1 + b2tg * b2 * b2 * c2 + b3tg * b3 * b3 * c3) +
	    24 * (b1t * b1g * b1 * c1 + b2t * b2g * b2 * c2 + b3t * b3g * b3 * c3) +
	    12 * (b1t * b1 * b1 * c1g + b2t * b2 * b2 * c2g + b3t * b3 * b3 * c3g) +
	    12 * (b1g * b1 * b1 * c1t + b2g * b2 * b2 * c2t + b3g * b3 * b3 * c3t) +
	    4 * (b1 * b1 * b1 * c1tg + b2 * b2 * b2 * c2tg + b3 * b3 * b3 * c3tg)) * coeffs[9];
    val += (12 * (b1tg * b1 * c1 * c1 + b2tg * b2 * c2 * c2 + b3tg * b3 * c3 * c3) +
	    12 * (b1t * b1g * c1 * c1 + b2t * b2g * c2 * c2 + b3t * b3g * c3 * c3) +
	    24 * (b1t * b1 * c1g * c1 + b2t * b2 * c2g * c2 + b3t * b3 * c3g * c3) +
	    24 * (b1g * b1 * c1t * c1 + b2g * b2 * c2t * c2 + b3g * b3 * c3t * c3) +
	    12 * (b1 * b1 * c1tg * c1 + b2 * b2 * c2tg * c2 + b3 * b3 * c3tg * c3) +
	    12 * (b1 * b1 * c1t * c1g + b2 * b2 * c2t * c2g + b3 * b3 * c3t * c3g)) * coeffs[10];
    val += (4 * (b1tg * c1 * c1 * c1 + b2tg * c2 * c2 * c2 + b3tg * c3 * c3 * c3) +
	    12 * (b1t * c1g * c1 * c1 + b2t * c2g * c2 * c2 + b3t * c3g * c3 * c3) +
	    12 * (b1g * c1t * c1 * c1 + b2g * c2t * c2 * c2 + b3g * c3t * c3 * c3) +
	    12 * (b1 * c1tg * c1 * c1 + b2 * c2tg * c2 * c2 + b3 * c3tg * c3 * c3) +
	    24 * (b1 * c1t * c1g * c1 + b2 * c2t * c2g * c2 + b3 * c3t * c3g * c3)) * coeffs[11];
    val += (24 * (a1tg * a1 * b1 * c1 + a2tg * a2 * b2 * c2 + a3tg * a3 * b3 * c3) +
	    24 * (a1t * a1g * b1 * c1 + a2t * a2g * b2 * c2 + a3t * a3g * b3 * c3) +
	    24 * (a1t * a1 * b1g * c1 + a2t * a2 * b2g * c2 + a3t * a3 * b3g * c3) +
	    24 * (a1t * a1 * b1 * c1g + a2t * a2 * b2 * c2g + a3t * a3 * b3 * c3g) +
	    24 * (a1g * a1 * b1t * c1 + a2g * a2 * b2t * c2 + a3g * a3 * b3t * c3) +
	    24 * (a1g * a1 * b1 * c1t + a2g * a2 * b2 * c2t + a3g * a3 * b3 * c3t) +
	    12 * (a1 * a1 * b1tg * c1 + a2 * a2 * b2tg * c2 + a3 * a3 * b3tg * c3) +
	    12 * (a1 * a1 * b1t * c1g + a2 * a2 * b2t * c2g + a3 * a3 * b3t * c3g) +
	    12 * (a1 * a1 * b1g * c1t + a2 * a2 * b2g * c2t + a3 * a3 * b3g * c3t) +
	    12 * (a1 * a1 * b1 * c1tg + a2 * a2 * b2 * c2tg + a3 * a3 * b3 * c3tg)) * coeffs[12];
    val += (12 * (a1tg * b1 * b1 * c1 + a2tg * b2 * b2 * c2 + a3tg * b3 * b3 * c3) +
	    24 * (a1t * b1g * b1 * c1 + a2t * b2g * b2 * c2 + a3t * b3g * b3 * c3) +
	    24 * (a1g * b1t * b1 * c1 + a2g * b2t * b2 * c2 + a3g * b3t * b3 * c3) +
	    12 * (a1t * b1 * b1 * c1g + a2t * b2 * b2 * c2g + a3t * b3 * b3 * c3g) +
	    12 * (a1g * b1 * b1 * c1t + a2g * b2 * b2 * c2t + a3g * b3 * b3 * c3t) +
	    24 * (a1 * b1tg * b1 * c1 + a2 * b2tg * b2 * c2 + a3 * b3tg * b3 * c3) +
	    24 * (a1 * b1t * b1g * c1 + a2 * b2t * b2g * c2 + a3 * b3t * b3g * c3) +
	    24 * (a1 * b1t * b1 * c1g + a2 * b2t * b2 * c2g + a3 * b3t * b3 * c3g) +
	    24 * (a1 * b1g * b1 * c1t + a2 * b2g * b2 * c2t + a3 * b3g * b3 * c3t) +
	    12 * (a1 * b1 * b1 * c1tg + a2 * b2 * b2 * c2tg + a3 * b3 * b3 * c3tg)) * coeffs[13];
    val += (12 * (a1tg * b1 * c1 * c1 + a2tg * b2 * c2 * c2 + a3tg * b3 * c3 * c3) +
	    12 * (a1t * b1g * c1 * c1 + a2t * b2g * c2 * c2 + a3t * b3g * c3 * c3) +
	    12 * (a1g * b1t * c1 * c1 + a2g * b2t * c2 * c2 + a3g * b3t * c3 * c3) +
	    24 * (a1t * b1 * c1g * c1 + a2t * b2 * c2g * c2 + a3t * b3 * c3g * c3) +
	    24 * (a1g * b1 * c1t * c1 + a2g * b2 * c2t * c2 + a3g * b3 * c3t * c3) +
	    12 * (a1 * b1tg * c1 * c1 + a2 * b2tg * c2 * c2 + a3 * b3tg * c3 * c3) +
	    24 * (a1 * b1t * c1g * c1 + a2 * b2t * c2g * c2 + a3 * b3t * c3g * c3) +
	    24 * (a1 * b1g * c1t * c1 + a2 * b2g * c2t * c2 + a3 * b3g * c3t * c3) +
	    24 * (a1 * b1 * c1tg * c1 + a2 * b2 * c2tg * c2 + a3 * b3 * c3tg * c3) +
	    24 * (a1 * b1 * c1t * c1g + a2 * b2 * c2t * c2g + a3 * b3 * c3t * c3g)) * coeffs[14];
    return (val);
}

static double hess12(coeffs)
double   *coeffs;
{
    double    val;

    val = (4 * (a1pg * a1 * a1 * a1 + a2pg * a2 * a2 * a2 + a3pg * a3 * a3 * a3) +
	   12 * (a1p * a1g * a1 * a1 + a2p * a2g * a2 * a2 + a3p * a3g * a3 * a3)) * coeffs[0];
    val += (4 * (b1pg * b1 * b1 * b1 + b2pg * b2 * b2 * b2 + b3pg * b3 * b3 * b3) +
	    12 * (b1p * b1g * b1 * b1 + b2p * b2g * b2 * b2 + b3p * b3g * b3 * b3)) * coeffs[1];
    val += (4 * (c1pg * c1 * c1 * c1 + c2pg * c2 * c2 * c2 + c3pg * c3 * c3 * c3) +
	    12 * (c1p * c1g * c1 * c1 + c2p * c2g * c2 * c2 + c3p * c3g * c3 * c3)) * coeffs[2];
    val += (12 * (a1pg * a1 * a1 * b1 + a2pg * a2 * a2 * b2 + a3pg * a3 * a3 * b3) +
	    24 * (a1p * a1g * a1 * b1 + a2p * a2g * a2 * b2 + a3p * a3g * a3 * b3) +
	    12 * (a1p * a1 * a1 * b1g + a2p * a2 * a2 * b2g + a3p * a3 * a3 * b3g) +
	    12 * (a1g * a1 * a1 * b1p + a2g * a2 * a2 * b2p + a3g * a3 * a3 * b3p) +
	    4 * (a1 * a1 * a1 * b1pg + a2 * a2 * a2 * b2pg + a3 * a3 * a3 * b3pg)) * coeffs[3];
    val += (12 * (a1pg * a1 * b1 * b1 + a2pg * a2 * b2 * b2 + a3pg * a3 * b3 * b3) +
	    12 * (a1p * a1g * b1 * b1 + a2p * a2g * b2 * b2 + a3p * a3g * b3 * b3) +
	    24 * (a1p * a1 * b1g * b1 + a2p * a2 * b2g * b2 + a3p * a3 * b3g * b3) +
	    24 * (a1g * a1 * b1p * b1 + a2g * a2 * b2p * b2 + a3g * a3 * b3p * b3) +
	    12 * (a1 * a1 * b1pg * b1 + a2 * a2 * b2pg * b2 + a3 * a3 * b3pg * b3) +
	    12 * (a1 * a1 * b1p * b1g + a2 * a2 * b2p * b2g + a3 * a3 * b3p * b3g)) * coeffs[4];
    val += (4 * (a1pg * b1 * b1 * b1 + a2pg * b2 * b2 * b2 + a3pg * b3 * b3 * b3) +
	    12 * (a1p * b1g * b1 * b1 + a2p * b2g * b2 * b2 + a3p * b3g * b3 * b3) +
	    12 * (a1g * b1p * b1 * b1 + a2g * b2p * b2 * b2 + a3g * b3p * b3 * b3) +
	    12 * (a1 * b1pg * b1 * b1 + a2 * b2pg * b2 * b2 + a3 * b3pg * b3 * b3) +
	    24 * (a1 * b1p * b1g * b1 + a2 * b2p * b2g * b2 + a3 * b3p * b3g * b3)) * coeffs[5];
    val += (12 * (a1pg * a1 * a1 * c1 + a2pg * a2 * a2 * c2 + a3pg * a3 * a3 * c3) +
	    24 * (a1p * a1g * a1 * c1 + a2p * a2g * a2 * c2 + a3p * a3g * a3 * c3) +
	    12 * (a1p * a1 * a1 * c1g + a2p * a2 * a2 * c2g + a3p * a3 * a3 * c3g) +
	    12 * (a1g * a1 * a1 * c1p + a2g * a2 * a2 * c2p + a3g * a3 * a3 * c3p) +
	    4 * (a1 * a1 * a1 * c1pg + a2 * a2 * a2 * c2pg + a3 * a3 * a3 * c3pg)) * coeffs[6];
    val += (12 * (a1pg * a1 * c1 * c1 + a2pg * a2 * c2 * c2 + a3pg * a3 * c3 * c3) +
	    12 * (a1p * a1g * c1 * c1 + a2p * a2g * c2 * c2 + a3p * a3g * c3 * c3) +
	    24 * (a1p * a1 * c1g * c1 + a2p * a2 * c2g * c2 + a3p * a3 * c3g * c3) +
	    24 * (a1g * a1 * c1p * c1 + a2g * a2 * c2p * c2 + a3g * a3 * c3p * c3) +
	    12 * (a1 * a1 * c1pg * c1 + a2 * a2 * c2pg * c2 + a3 * a3 * c3pg * c3) +
	    12 * (a1 * a1 * c1p * c1g + a2 * a2 * c2p * c2g + a3 * a3 * c3p * c3g)) * coeffs[7];
    val += (4 * (a1pg * c1 * c1 * c1 + a2pg * c2 * c2 * c2 + a3pg * c3 * c3 * c3) +
	    12 * (a1p * c1g * c1 * c1 + a2p * c2g * c2 * c2 + a3p * c3g * c3 * c3) +
	    12 * (a1g * c1p * c1 * c1 + a2g * c2p * c2 * c2 + a3g * c3p * c3 * c3) +
	    12 * (a1 * c1pg * c1 * c1 + a2 * c2pg * c2 * c2 + a3 * c3pg * c3 * c3) +
	    24 * (a1 * c1p * c1g * c1 + a2 * c2p * c2g * c2 + a3 * c3p * c3g * c3)) * coeffs[8];
    val += (12 * (b1pg * b1 * b1 * c1 + b2pg * b2 * b2 * c2 + b3pg * b3 * b3 * c3) +
	    24 * (b1p * b1g * b1 * c1 + b2p * b2g * b2 * c2 + b3p * b3g * b3 * c3) +
	    12 * (b1p * b1 * b1 * c1g + b2p * b2 * b2 * c2g + b3p * b3 * b3 * c3g) +
	    12 * (b1g * b1 * b1 * c1p + b2g * b2 * b2 * c2p + b3g * b3 * b3 * c3p) +
	    4 * (b1 * b1 * b1 * c1pg + b2 * b2 * b2 * c2pg + b3 * b3 * b3 * c3pg)) * coeffs[9];
    val += (12 * (b1pg * b1 * c1 * c1 + b2pg * b2 * c2 * c2 + b3pg * b3 * c3 * c3) +
	    12 * (b1p * b1g * c1 * c1 + b2p * b2g * c2 * c2 + b3p * b3g * c3 * c3) +
	    24 * (b1p * b1 * c1g * c1 + b2p * b2 * c2g * c2 + b3p * b3 * c3g * c3) +
	    24 * (b1g * b1 * c1p * c1 + b2g * b2 * c2p * c2 + b3g * b3 * c3p * c3) +
	    12 * (b1 * b1 * c1pg * c1 + b2 * b2 * c2pg * c2 + b3 * b3 * c3pg * c3) +
	    12 * (b1 * b1 * c1p * c1g + b2 * b2 * c2p * c2g + b3 * b3 * c3p * c3g)) * coeffs[10];
    val += (4 * (b1pg * c1 * c1 * c1 + b2pg * c2 * c2 * c2 + b3pg * c3 * c3 * c3) +
	    12 * (b1p * c1g * c1 * c1 + b2p * c2g * c2 * c2 + b3p * c3g * c3 * c3) +
	    12 * (b1g * c1p * c1 * c1 + b2g * c2p * c2 * c2 + b3g * c3p * c3 * c3) +
	    12 * (b1 * c1pg * c1 * c1 + b2 * c2pg * c2 * c2 + b3 * c3pg * c3 * c3) +
	    24 * (b1 * c1p * c1g * c1 + b2 * c2p * c2g * c2 + b3 * c3p * c3g * c3)) * coeffs[11];
    val += (24 * (a1pg * a1 * b1 * c1 + a2pg * a2 * b2 * c2 + a3pg * a3 * b3 * c3) +
	    24 * (a1p * a1g * b1 * c1 + a2p * a2g * b2 * c2 + a3p * a3g * b3 * c3) +
	    24 * (a1p * a1 * b1g * c1 + a2p * a2 * b2g * c2 + a3p * a3 * b3g * c3) +
	    24 * (a1g * a1 * b1p * c1 + a2g * a2 * b2p * c2 + a3g * a3 * b3p * c3) +
	    24 * (a1p * a1 * b1 * c1g + a2p * a2 * b2 * c2g + a3p * a3 * b3 * c3g) +
	    24 * (a1g * a1 * b1 * c1p + a2g * a2 * b2 * c2p + a3g * a3 * b3 * c3p) +
	    12 * (a1 * a1 * b1pg * c1 + a2 * a2 * b2pg * c2 + a3 * a3 * b3pg * c3) +
	    12 * (a1 * a1 * b1p * c1g + a2 * a2 * b2p * c2g + a3 * a3 * b3p * c3g) +
	    12 * (a1 * a1 * b1g * c1p + a2 * a2 * b2g * c2p + a3 * a3 * b3g * c3p) +
	    12 * (a1 * a1 * b1 * c1pg + a2 * a2 * b2 * c2pg + a3 * a3 * b3 * c3pg)) * coeffs[12];
    val += (12 * (a1pg * b1 * b1 * c1 + a2pg * b2 * b2 * c2 + a3pg * b3 * b3 * c3) +
	    24 * (a1p * b1g * b1 * c1 + a2p * b2g * b2 * c2 + a3p * b3g * b3 * c3) +
	    24 * (a1g * b1p * b1 * c1 + a2g * b2p * b2 * c2 + a3g * b3p * b3 * c3) +
	    12 * (a1p * b1 * b1 * c1g + a2p * b2 * b2 * c2g + a3p * b3 * b3 * c3g) +
	    12 * (a1g * b1 * b1 * c1p + a2g * b2 * b2 * c2p + a3g * b3 * b3 * c3p) +
	    24 * (a1 * b1pg * b1 * c1 + a2 * b2pg * b2 * c2 + a3 * b3pg * b3 * c3) +
	    24 * (a1 * b1p * b1g * c1 + a2 * b2p * b2g * c2 + a3 * b3p * b3g * c3) +
	    24 * (a1 * b1p * b1 * c1g + a2 * b2p * b2 * c2g + a3 * b3p * b3 * c3g) +
	    24 * (a1 * b1g * b1 * c1p + a2 * b2g * b2 * c2p + a3 * b3g * b3 * c3p) +
	    12 * (a1 * b1 * b1 * c1pg + a2 * b2 * b2 * c2pg + a3 * b3 * b3 * c3pg)) * coeffs[13];
    val += (12 * (a1pg * b1 * c1 * c1 + a2pg * b2 * c2 * c2 + a3pg * b3 * c3 * c3) +
	    12 * (a1p * b1g * c1 * c1 + a2p * b2g * c2 * c2 + a3p * b3g * c3 * c3) +
	    12 * (a1g * b1p * c1 * c1 + a2g * b2p * c2 * c2 + a3g * b3p * c3 * c3) +
	    24 * (a1p * b1 * c1g * c1 + a2p * b2 * c2g * c2 + a3p * b3 * c3g * c3) +
	    24 * (a1g * b1 * c1p * c1 + a2g * b2 * c2p * c2 + a3g * b3 * c3p * c3) +
	    12 * (a1 * b1pg * c1 * c1 + a2 * b2pg * c2 * c2 + a3 * b3pg * c3 * c3) +
	    24 * (a1 * b1p * c1g * c1 + a2 * b2p * c2g * c2 + a3 * b3p * c3g * c3) +
	    24 * (a1 * b1g * c1p * c1 + a2 * b2g * c2p * c2 + a3 * b3g * c3p * c3) +
	    24 * (a1 * b1 * c1pg * c1 + a2 * b2 * c2pg * c2 + a3 * b3 * c3pg * c3) +
	    24 * (a1 * b1 * c1p * c1g + a2 * b2 * c2p * c2g + a3 * b3 * c3p * c3g)) * coeffs[14];
    return (val);
}


double    constraint(coeffs2)
double   *coeffs2;		/* coefficients for constraint eqn */
{
    double    val;		/* value of constraint (should be zero) */

    val = a1 * a2 * a3 * coeffs2[0];
    val += b1 * b2 * b3 * coeffs2[1];
    val += c1 * c2 * c3 * coeffs2[2];
    val += (a1 * a2 * b3 + a1 * b2 * a3 + b1 * a2 * a3) * coeffs2[3];
    val += (a1 * a2 * c3 + a1 * c2 * a3 + c1 * a2 * a3) * coeffs2[4];
    val += (a1 * b2 * b3 + b1 * a2 * b3 + b1 * b2 * a3) * coeffs2[5];
    val += (b1 * b2 * c3 + b1 * c2 * b3 + c1 * b2 * b3) * coeffs2[6];
    val += (a1 * c2 * c3 + c1 * a2 * c3 + c1 * c2 * a3) * coeffs2[7];
    val += (b1 * c2 * c3 + c1 * b2 * c3 + c1 * c2 * b3) * coeffs2[8];
    val += (a1 * b2 * c3 + a1 * c2 * b3 + b1 * a2 * c3 +
	    b1 * c2 * a3 + c1 * a2 * b3 + c1 * b2 * a3) * coeffs2[9];

    return (val * val);
}


static double gradcon0(), gradcon1(), gradcon2();

void      gradcon(coeffs2, grad)
double   *coeffs2;		/* coefficients for constraint eqn */
double    grad[3];		/* gradient returned */
{
    int       i;		/* loop counter */

    cval = a1 * a2 * a3 * coeffs2[0];
    cval += b1 * b2 * b3 * coeffs2[1];
    cval += c1 * c2 * c3 * coeffs2[2];
    cval += (a1 * a2 * b3 + a1 * b2 * a3 + b1 * a2 * a3) * coeffs2[3];
    cval += (a1 * a2 * c3 + a1 * c2 * a3 + c1 * a2 * a3) * coeffs2[4];
    cval += (a1 * b2 * b3 + b1 * a2 * b3 + b1 * b2 * a3) * coeffs2[5];
    cval += (b1 * b2 * c3 + b1 * c2 * b3 + c1 * b2 * b3) * coeffs2[6];
    cval += (a1 * c2 * c3 + c1 * a2 * c3 + c1 * c2 * a3) * coeffs2[7];
    cval += (b1 * c2 * c3 + c1 * b2 * c3 + c1 * c2 * b3) * coeffs2[8];
    cval += (a1 * b2 * c3 + a1 * c2 * b3 + b1 * a2 * c3 + b1 * c2 * a3 + c1 * a2 * b3 +
	     c1 * b2 * a3) * coeffs2[9];

    cgrad[0] = gradcon0(coeffs2);
    cgrad[1] = gradcon1(coeffs2);
    cgrad[2] = gradcon2(coeffs2);

    for (i = 0; i < 3; i++)
	grad[i] = 2 * cval * cgrad[i];
}

static double gradcon0(coeffs2)
double   *coeffs2;
{
    double    val;

    val = (a1t * a2 * a3 + a1 * a2t * a3 + a1 * a2 * a3t) * coeffs2[0];
    val += (b1t * b2 * b3 + b1 * b2t * b3 + b1 * b2 * b3t) * coeffs2[1];
    val += (c1t * c2 * c3 + c1 * c2t * c3 + c1 * c2 * c3t) * coeffs2[2];
    val += (a1t * a2 * b3 + a1t * b2 * a3 + b1t * a2 * a3 +
	    a1 * a2t * b3 + a1 * b2t * a3 + b1 * a2t * a3 +
	    a1 * a2 * b3t + a1 * b2 * a3t + b1 * a2 * a3t) * coeffs2[3];
    val += (a1t * a2 * c3 + a1t * c2 * a3 + c1t * a2 * a3 +
	    a1 * a2t * c3 + a1 * c2t * a3 + c1 * a2t * a3 +
	    a1 * a2 * c3t + a1 * c2 * a3t + c1 * a2 * a3t) * coeffs2[4];
    val += (a1t * b2 * b3 + b1t * a2 * b3 + b1t * b2 * a3 +
	    a1 * b2t * b3 + b1 * a2t * b3 + b1 * b2t * a3 +
	    a1 * b2 * b3t + b1 * a2 * b3t + b1 * b2 * a3t) * coeffs2[5];
    val += (b1t * b2 * c3 + b1t * c2 * b3 + c1t * b2 * b3 +
	    b1 * b2t * c3 + b1 * c2t * b3 + c1 * b2t * b3 +
	    b1 * b2 * c3t + b1 * c2 * b3t + c1 * b2 * b3t) * coeffs2[6];
    val += (a1t * c2 * c3 + c1t * a2 * c3 + c1t * c2 * a3 +
	    a1 * c2t * c3 + c1 * a2t * c3 + c1 * c2t * a3 +
	    a1 * c2 * c3t + c1 * a2 * c3t + c1 * c2 * a3t) * coeffs2[7];
    val += (b1t * c2 * c3 + c1t * b2 * c3 + c1t * c2 * b3 +
	    b1 * c2t * c3 + c1 * b2t * c3 + c1 * c2t * b3 +
	    b1 * c2 * c3t + c1 * b2 * c3t + c1 * c2 * b3t) * coeffs2[8];
    val += (a1t * b2 * c3 + a1t * c2 * b3 + b1t * a2 * c3 +
	    a1 * b2t * c3 + a1 * c2t * b3 + b1 * a2t * c3 +
	    a1 * b2 * c3t + a1 * c2 * b3t + b1 * a2 * c3t +
	    b1t * c2 * a3 + c1t * a2 * b3 + c1t * b2 * a3 +
	    b1 * c2t * a3 + c1 * a2t * b3 + c1 * b2t * a3 +
	    b1 * c2 * a3t + c1 * a2 * b3t + c1 * b2 * a3t) * coeffs2[9];
    return (val);
}

static double gradcon1(coeffs2)
double   *coeffs2;
{
    double    val;

    val = (a1p * a2 * a3 + a1 * a2p * a3 + a1 * a2 * a3p) * coeffs2[0];
    val += (b1p * b2 * b3 + b1 * b2p * b3 + b1 * b2 * b3p) * coeffs2[1];
    val += (c1p * c2 * c3 + c1 * c2p * c3 + c1 * c2 * c3p) * coeffs2[2];
    val += (a1p * a2 * b3 + a1p * b2 * a3 + b1p * a2 * a3 +
	    a1 * a2p * b3 + a1 * b2p * a3 + b1 * a2p * a3 +
	    a1 * a2 * b3p + a1 * b2 * a3p + b1 * a2 * a3p) * coeffs2[3];
    val += (a1p * a2 * c3 + a1p * c2 * a3 + c1p * a2 * a3 +
	    a1 * a2p * c3 + a1 * c2p * a3 + c1 * a2p * a3 +
	    a1 * a2 * c3p + a1 * c2 * a3p + c1 * a2 * a3p) * coeffs2[4];
    val += (a1p * b2 * b3 + b1p * a2 * b3 + b1p * b2 * a3 +
	    a1 * b2p * b3 + b1 * a2p * b3 + b1 * b2p * a3 +
	    a1 * b2 * b3p + b1 * a2 * b3p + b1 * b2 * a3p) * coeffs2[5];
    val += (b1p * b2 * c3 + b1p * c2 * b3 + c1p * b2 * b3 +
	    b1 * b2p * c3 + b1 * c2p * b3 + c1 * b2p * b3 +
	    b1 * b2 * c3p + b1 * c2 * b3p + c1 * b2 * b3p) * coeffs2[6];
    val += (a1p * c2 * c3 + c1p * a2 * c3 + c1p * c2 * a3 +
	    a1 * c2p * c3 + c1 * a2p * c3 + c1 * c2p * a3 +
	    a1 * c2 * c3p + c1 * a2 * c3p + c1 * c2 * a3p) * coeffs2[7];
    val += (b1p * c2 * c3 + c1p * b2 * c3 + c1p * c2 * b3 +
	    b1 * c2p * c3 + c1 * b2p * c3 + c1 * c2p * b3 +
	    b1 * c2 * c3p + c1 * b2 * c3p + c1 * c2 * b3p) * coeffs2[8];
    val += (a1p * b2 * c3 + a1p * c2 * b3 + b1p * a2 * c3 +
	    a1 * b2p * c3 + a1 * c2p * b3 + b1 * a2p * c3 +
	    a1 * b2 * c3p + a1 * c2 * b3p + b1 * a2 * c3p +
	    b1p * c2 * a3 + c1p * a2 * b3 + c1p * b2 * a3 +
	    b1 * c2p * a3 + c1 * a2p * b3 + c1 * b2p * a3 +
	    b1 * c2 * a3p + c1 * a2 * b3p + c1 * b2 * a3p) * coeffs2[9];
    return (val);
}

static double gradcon2(coeffs2)
double   *coeffs2;
{
    double    val;

    val = (a1g * a2 * a3 + a1 * a2g * a3 + a1 * a2 * a3g) * coeffs2[0];
    val += (b1g * b2 * b3 + b1 * b2g * b3 + b1 * b2 * b3g) * coeffs2[1];
    val += (c1g * c2 * c3 + c1 * c2g * c3 + c1 * c2 * c3g) * coeffs2[2];
    val += (a1g * a2 * b3 + a1g * b2 * a3 + b1g * a2 * a3 +
	    a1 * a2g * b3 + a1 * b2g * a3 + b1 * a2g * a3 +
	    a1 * a2 * b3g + a1 * b2 * a3g + b1 * a2 * a3g) * coeffs2[3];
    val += (a1g * a2 * c3 + a1g * c2 * a3 + c1g * a2 * a3 +
	    a1 * a2g * c3 + a1 * c2g * a3 + c1 * a2g * a3 +
	    a1 * a2 * c3g + a1 * c2 * a3g + c1 * a2 * a3g) * coeffs2[4];
    val += (a1g * b2 * b3 + b1g * a2 * b3 + b1g * b2 * a3 +
	    a1 * b2g * b3 + b1 * a2g * b3 + b1 * b2g * a3 +
	    a1 * b2 * b3g + b1 * a2 * b3g + b1 * b2 * a3g) * coeffs2[5];
    val += (b1g * b2 * c3 + b1g * c2 * b3 + c1g * b2 * b3 +
	    b1 * b2g * c3 + b1 * c2g * b3 + c1 * b2g * b3 +
	    b1 * b2 * c3g + b1 * c2 * b3g + c1 * b2 * b3g) * coeffs2[6];
    val += (a1g * c2 * c3 + c1g * a2 * c3 + c1g * c2 * a3 +
	    a1 * c2g * c3 + c1 * a2g * c3 + c1 * c2g * a3 +
	    a1 * c2 * c3g + c1 * a2 * c3g + c1 * c2 * a3g) * coeffs2[7];
    val += (b1g * c2 * c3 + c1g * b2 * c3 + c1g * c2 * b3 +
	    b1 * c2g * c3 + c1 * b2g * c3 + c1 * c2g * b3 +
	    b1 * c2 * c3g + c1 * b2 * c3g + c1 * c2 * b3g) * coeffs2[8];
    val += (a1g * b2 * c3 + a1g * c2 * b3 + b1g * a2 * c3 +
	    a1 * b2g * c3 + a1 * c2g * b3 + b1 * a2g * c3 +
	    a1 * b2 * c3g + a1 * c2 * b3g + b1 * a2 * c3g +
	    b1g * c2 * a3 + c1g * a2 * b3 + c1g * b2 * a3 +
	    b1 * c2g * a3 + c1 * a2g * b3 + c1 * b2g * a3 +
	    b1 * c2 * a3g + c1 * a2 * b3g + c1 * b2 * a3g) * coeffs2[9];
    return (val);
}


static double hesscon00(), hesscon11(), hesscon22();
static double hesscon01(), hesscon02(), hesscon12();

void      hesscon(coeffs2, hess)
double   *coeffs2;		/* coefficients for constraint eqn */
double    hess[3][3];		/* hessian returned */
{
    int       i, j;		/* loop variables */

    hess[0][0] = hesscon00(coeffs2);
    hess[1][1] = hesscon11(coeffs2);
    hess[2][2] = hesscon22(coeffs2);
    hess[0][1] = hesscon01(coeffs2);
    hess[0][2] = hesscon02(coeffs2);
    hess[1][2] = hesscon12(coeffs2);

    /* Now adjust for f^2 instead of f, and make Hessian symmetric. */
    for (i = 0; i < 3; i++) {
	for (j = i; j < 3; j++) {
	    hess[i][j] = 2 * (cval * hess[i][j] + cgrad[i] * cgrad[j]);
	}
    }
    hess[1][0] = hess[0][1];
    hess[2][0] = hess[0][2];
    hess[2][1] = hess[1][2];
}

static double hesscon00(coeffs2)
double   *coeffs2;
{
    double    val;

    val = (a1tt * a2 * a3 + a1t * a2t * a3 + a1t * a2 * a3t +
	   a1t * a2t * a3 + a1 * a2tt * a3 + a1 * a2t * a3t +
	   a1t * a2 * a3t + a1 * a2t * a3t + a1 * a2 * a3tt) * coeffs2[0];
    val += (b1tt * b2 * b3 + b1t * b2t * b3 + b1t * b2 * b3t +
	    b1t * b2t * b3 + b1 * b2tt * b3 + b1 * b2t * b3t +
	    b1t * b2 * b3t + b1 * b2t * b3t + b1 * b2 * b3tt) * coeffs2[1];
    val += (c1tt * c2 * c3 + c1t * c2t * c3 + c1t * c2 * c3t +
	    c1t * c2t * c3 + c1 * c2tt * c3 + c1 * c2t * c3t +
	    c1t * c2 * c3t + c1 * c2t * c3t + c1 * c2 * c3tt) * coeffs2[2];
    val += (a1tt * a2 * b3 + a1tt * b2 * a3 + b1tt * a2 * a3 +
	    a1t * a2t * b3 + a1t * b2t * a3 + b1t * a2t * a3 +
	    a1t * a2 * b3t + a1t * b2 * a3t + b1t * a2 * a3t +
	    a1t * a2t * b3 + a1t * b2t * a3 + b1t * a2t * a3 +
	    a1 * a2tt * b3 + a1 * b2tt * a3 + b1 * a2tt * a3 +
	    a1 * a2t * b3t + a1 * b2t * a3t + b1 * a2t * a3t +
	    a1t * a2 * b3t + a1t * b2 * a3t + b1t * a2 * a3t +
	    a1 * a2t * b3t + a1 * b2t * a3t + b1 * a2t * a3t +
	    a1 * a2 * b3tt + a1 * b2 * a3tt + b1 * a2 * a3tt) * coeffs2[3];
    val += (a1tt * a2 * c3 + a1tt * c2 * a3 + c1tt * a2 * a3 +
	    a1t * a2t * c3 + a1t * c2t * a3 + c1t * a2t * a3 +
	    a1t * a2 * c3t + a1t * c2 * a3t + c1t * a2 * a3t +
	    a1t * a2t * c3 + a1t * c2t * a3 + c1t * a2t * a3 +
	    a1 * a2tt * c3 + a1 * c2tt * a3 + c1 * a2tt * a3 +
	    a1 * a2t * c3t + a1 * c2t * a3t + c1 * a2t * a3t +
	    a1t * a2 * c3t + a1t * c2 * a3t + c1t * a2 * a3t +
	    a1 * a2t * c3t + a1 * c2t * a3t + c1 * a2t * a3t +
	    a1 * a2 * c3tt + a1 * c2 * a3tt + c1 * a2 * a3tt) * coeffs2[4];
    val += (a1tt * b2 * b3 + b1tt * a2 * b3 + b1tt * b2 * a3 +
	    a1t * b2t * b3 + b1t * a2t * b3 + b1t * b2t * a3 +
	    a1t * b2 * b3t + b1t * a2 * b3t + b1t * b2 * a3t +
	    a1t * b2t * b3 + b1t * a2t * b3 + b1t * b2t * a3 +
	    a1 * b2tt * b3 + b1 * a2tt * b3 + b1 * b2tt * a3 +
	    a1 * b2t * b3t + b1 * a2t * b3t + b1 * b2t * a3t +
	    a1t * b2 * b3t + b1t * a2 * b3t + b1t * b2 * a3t +
	    a1 * b2t * b3t + b1 * a2t * b3t + b1 * b2t * a3t +
	    a1 * b2 * b3tt + b1 * a2 * b3tt + b1 * b2 * a3tt) * coeffs2[5];
    val += (b1tt * b2 * c3 + b1tt * c2 * b3 + c1tt * b2 * b3 +
	    b1t * b2t * c3 + b1t * c2t * b3 + c1t * b2t * b3 +
	    b1t * b2 * c3t + b1t * c2 * b3t + c1t * b2 * b3t +
	    b1t * b2t * c3 + b1t * c2t * b3 + c1t * b2t * b3 +
	    b1 * b2tt * c3 + b1 * c2tt * b3 + c1 * b2tt * b3 +
	    b1 * b2t * c3t + b1 * c2t * b3t + c1 * b2t * b3t +
	    b1t * b2 * c3t + b1t * c2 * b3t + c1t * b2 * b3t +
	    b1 * b2t * c3t + b1 * c2t * b3t + c1 * b2t * b3t +
	    b1 * b2 * c3tt + b1 * c2 * b3tt + c1 * b2 * b3tt) * coeffs2[6];
    val += (a1tt * c2 * c3 + c1tt * a2 * c3 + c1tt * c2 * a3 +
	    a1t * c2t * c3 + c1t * a2t * c3 + c1t * c2t * a3 +
	    a1t * c2 * c3t + c1t * a2 * c3t + c1t * c2 * a3t +
	    a1t * c2t * c3 + c1t * a2t * c3 + c1t * c2t * a3 +
	    a1 * c2tt * c3 + c1 * a2tt * c3 + c1 * c2tt * a3 +
	    a1 * c2t * c3t + c1 * a2t * c3t + c1 * c2t * a3t +
	    a1t * c2 * c3t + c1t * a2 * c3t + c1t * c2 * a3t +
	    a1 * c2t * c3t + c1 * a2t * c3t + c1 * c2t * a3t +
	    a1 * c2 * c3tt + c1 * a2 * c3tt + c1 * c2 * a3tt) * coeffs2[7];
    val += (b1tt * c2 * c3 + c1tt * b2 * c3 + c1tt * c2 * b3 +
	    b1t * c2t * c3 + c1t * b2t * c3 + c1t * c2t * b3 +
	    b1t * c2 * c3t + c1t * b2 * c3t + c1t * c2 * b3t +
	    b1t * c2t * c3 + c1t * b2t * c3 + c1t * c2t * b3 +
	    b1 * c2tt * c3 + c1 * b2tt * c3 + c1 * c2tt * b3 +
	    b1 * c2t * c3t + c1 * b2t * c3t + c1 * c2t * b3t +
	    b1t * c2 * c3t + c1t * b2 * c3t + c1t * c2 * b3t +
	    b1 * c2t * c3t + c1 * b2t * c3t + c1 * c2t * b3t +
	    b1 * c2 * c3tt + c1 * b2 * c3tt + c1 * c2 * b3tt) * coeffs2[8];
    val += (a1tt * b2 * c3 + a1tt * c2 * b3 + b1tt * a2 * c3 +
	    a1t * b2t * c3 + a1t * c2t * b3 + b1t * a2t * c3 +
	    a1t * b2 * c3t + a1t * c2 * b3t + b1t * a2 * c3t +
	    a1t * b2t * c3 + a1t * c2t * b3 + b1t * a2t * c3 +
	    a1 * b2tt * c3 + a1 * c2tt * b3 + b1 * a2tt * c3 +
	    a1 * b2t * c3t + a1 * c2t * b3t + b1 * a2t * c3t +
	    a1t * b2 * c3t + a1t * c2 * b3t + b1t * a2 * c3t +
	    a1 * b2t * c3t + a1 * c2t * b3t + b1 * a2t * c3t +
	    a1 * b2 * c3tt + a1 * c2 * b3tt + b1 * a2 * c3tt +
	    b1tt * c2 * a3 + c1tt * a2 * b3 + c1tt * b2 * a3 +
	    b1t * c2t * a3 + c1t * a2t * b3 + c1t * b2t * a3 +
	    b1t * c2 * a3t + c1t * a2 * b3t + c1t * b2 * a3t +
	    b1t * c2t * a3 + c1t * a2t * b3 + c1t * b2t * a3 +
	    b1 * c2tt * a3 + c1 * a2tt * b3 + c1 * b2tt * a3 +
	    b1 * c2t * a3t + c1 * a2t * b3t + c1 * b2t * a3t +
	    b1t * c2 * a3t + c1t * a2 * b3t + c1t * b2 * a3t +
	    b1 * c2t * a3t + c1 * a2t * b3t + c1 * b2t * a3t +
	    b1 * c2 * a3tt + c1 * a2 * b3tt + c1 * b2 * a3tt) * coeffs2[9];
    return (val);
}

static double hesscon11(coeffs2)
double   *coeffs2;
{
    double    val;

    val = (a1pp * a2 * a3 + a1p * a2p * a3 + a1p * a2 * a3p +
	   a1p * a2p * a3 + a1 * a2pp * a3 + a1 * a2p * a3p +
	   a1p * a2 * a3p + a1 * a2p * a3p + a1 * a2 * a3pp) * coeffs2[0];
    val += (b1pp * b2 * b3 + b1p * b2p * b3 + b1p * b2 * b3p +
	    b1p * b2p * b3 + b1 * b2pp * b3 + b1 * b2p * b3p +
	    b1p * b2 * b3p + b1 * b2p * b3p + b1 * b2 * b3pp) * coeffs2[1];
    val += (c1pp * c2 * c3 + c1p * c2p * c3 + c1p * c2 * c3p +
	    c1p * c2p * c3 + c1 * c2pp * c3 + c1 * c2p * c3p +
	    c1p * c2 * c3p + c1 * c2p * c3p + c1 * c2 * c3pp) * coeffs2[2];
    val += (a1pp * a2 * b3 + a1pp * b2 * a3 + b1pp * a2 * a3 +
	    a1p * a2p * b3 + a1p * b2p * a3 + b1p * a2p * a3 +
	    a1p * a2 * b3p + a1p * b2 * a3p + b1p * a2 * a3p +
	    a1p * a2p * b3 + a1p * b2p * a3 + b1p * a2p * a3 +
	    a1 * a2pp * b3 + a1 * b2pp * a3 + b1 * a2pp * a3 +
	    a1 * a2p * b3p + a1 * b2p * a3p + b1 * a2p * a3p +
	    a1p * a2 * b3p + a1p * b2 * a3p + b1p * a2 * a3p +
	    a1 * a2p * b3p + a1 * b2p * a3p + b1 * a2p * a3p +
	    a1 * a2 * b3pp + a1 * b2 * a3pp + b1 * a2 * a3pp) * coeffs2[3];
    val += (a1pp * a2 * c3 + a1pp * c2 * a3 + c1pp * a2 * a3 +
	    a1p * a2p * c3 + a1p * c2p * a3 + c1p * a2p * a3 +
	    a1p * a2 * c3p + a1p * c2 * a3p + c1p * a2 * a3p +
	    a1p * a2p * c3 + a1p * c2p * a3 + c1p * a2p * a3 +
	    a1 * a2pp * c3 + a1 * c2pp * a3 + c1 * a2pp * a3 +
	    a1 * a2p * c3p + a1 * c2p * a3p + c1 * a2p * a3p +
	    a1p * a2 * c3p + a1p * c2 * a3p + c1p * a2 * a3p +
	    a1 * a2p * c3p + a1 * c2p * a3p + c1 * a2p * a3p +
	    a1 * a2 * c3pp + a1 * c2 * a3pp + c1 * a2 * a3pp) * coeffs2[4];
    val += (a1pp * b2 * b3 + b1pp * a2 * b3 + b1pp * b2 * a3 +
	    a1p * b2p * b3 + b1p * a2p * b3 + b1p * b2p * a3 +
	    a1p * b2 * b3p + b1p * a2 * b3p + b1p * b2 * a3p +
	    a1p * b2p * b3 + b1p * a2p * b3 + b1p * b2p * a3 +
	    a1 * b2pp * b3 + b1 * a2pp * b3 + b1 * b2pp * a3 +
	    a1 * b2p * b3p + b1 * a2p * b3p + b1 * b2p * a3p +
	    a1p * b2 * b3p + b1p * a2 * b3p + b1p * b2 * a3p +
	    a1 * b2p * b3p + b1 * a2p * b3p + b1 * b2p * a3p +
	    a1 * b2 * b3pp + b1 * a2 * b3pp + b1 * b2 * a3pp) * coeffs2[5];
    val += (b1pp * b2 * c3 + b1pp * c2 * b3 + c1pp * b2 * b3 +
	    b1p * b2p * c3 + b1p * c2p * b3 + c1p * b2p * b3 +
	    b1p * b2 * c3p + b1p * c2 * b3p + c1p * b2 * b3p +
	    b1p * b2p * c3 + b1p * c2p * b3 + c1p * b2p * b3 +
	    b1 * b2pp * c3 + b1 * c2pp * b3 + c1 * b2pp * b3 +
	    b1 * b2p * c3p + b1 * c2p * b3p + c1 * b2p * b3p +
	    b1p * b2 * c3p + b1p * c2 * b3p + c1p * b2 * b3p +
	    b1 * b2p * c3p + b1 * c2p * b3p + c1 * b2p * b3p +
	    b1 * b2 * c3pp + b1 * c2 * b3pp + c1 * b2 * b3pp) * coeffs2[6];
    val += (a1pp * c2 * c3 + c1pp * a2 * c3 + c1pp * c2 * a3 +
	    a1p * c2p * c3 + c1p * a2p * c3 + c1p * c2p * a3 +
	    a1p * c2 * c3p + c1p * a2 * c3p + c1p * c2 * a3p +
	    a1p * c2p * c3 + c1p * a2p * c3 + c1p * c2p * a3 +
	    a1 * c2pp * c3 + c1 * a2pp * c3 + c1 * c2pp * a3 +
	    a1 * c2p * c3p + c1 * a2p * c3p + c1 * c2p * a3p +
	    a1p * c2 * c3p + c1p * a2 * c3p + c1p * c2 * a3p +
	    a1 * c2p * c3p + c1 * a2p * c3p + c1 * c2p * a3p +
	    a1 * c2 * c3pp + c1 * a2 * c3pp + c1 * c2 * a3pp) * coeffs2[7];
    val += (b1pp * c2 * c3 + c1pp * b2 * c3 + c1pp * c2 * b3 +
	    b1p * c2p * c3 + c1p * b2p * c3 + c1p * c2p * b3 +
	    b1p * c2 * c3p + c1p * b2 * c3p + c1p * c2 * b3p +
	    b1p * c2p * c3 + c1p * b2p * c3 + c1p * c2p * b3 +
	    b1 * c2pp * c3 + c1 * b2pp * c3 + c1 * c2pp * b3 +
	    b1 * c2p * c3p + c1 * b2p * c3p + c1 * c2p * b3p +
	    b1p * c2 * c3p + c1p * b2 * c3p + c1p * c2 * b3p +
	    b1 * c2p * c3p + c1 * b2p * c3p + c1 * c2p * b3p +
	    b1 * c2 * c3pp + c1 * b2 * c3pp + c1 * c2 * b3pp) * coeffs2[8];
    val += (a1pp * b2 * c3 + a1pp * c2 * b3 + b1pp * a2 * c3 +
	    a1p * b2p * c3 + a1p * c2p * b3 + b1p * a2p * c3 +
	    a1p * b2 * c3p + a1p * c2 * b3p + b1p * a2 * c3p +
	    a1p * b2p * c3 + a1p * c2p * b3 + b1p * a2p * c3 +
	    a1 * b2pp * c3 + a1 * c2pp * b3 + b1 * a2pp * c3 +
	    a1 * b2p * c3p + a1 * c2p * b3p + b1 * a2p * c3p +
	    a1p * b2 * c3p + a1p * c2 * b3p + b1p * a2 * c3p +
	    a1 * b2p * c3p + a1 * c2p * b3p + b1 * a2p * c3p +
	    a1 * b2 * c3pp + a1 * c2 * b3pp + b1 * a2 * c3pp +
	    b1pp * c2 * a3 + c1pp * a2 * b3 + c1pp * b2 * a3 +
	    b1p * c2p * a3 + c1p * a2p * b3 + c1p * b2p * a3 +
	    b1p * c2 * a3p + c1p * a2 * b3p + c1p * b2 * a3p +
	    b1p * c2p * a3 + c1p * a2p * b3 + c1p * b2p * a3 +
	    b1 * c2pp * a3 + c1 * a2pp * b3 + c1 * b2pp * a3 +
	    b1 * c2p * a3p + c1 * a2p * b3p + c1 * b2p * a3p +
	    b1p * c2 * a3p + c1p * a2 * b3p + c1p * b2 * a3p +
	    b1 * c2p * a3p + c1 * a2p * b3p + c1 * b2p * a3p +
	    b1 * c2 * a3pp + c1 * a2 * b3pp + c1 * b2 * a3pp) * coeffs2[9];
    return (val);
}

static double hesscon22(coeffs2)
double   *coeffs2;
{
    double    val;

    val = (a1gg * a2 * a3 + a1g * a2g * a3 + a1g * a2 * a3g +
	   a1g * a2g * a3 + a1 * a2gg * a3 + a1 * a2g * a3g +
	   a1g * a2 * a3g + a1 * a2g * a3g + a1 * a2 * a3gg) * coeffs2[0];
    val += (b1gg * b2 * b3 + b1g * b2g * b3 + b1g * b2 * b3g +
	    b1g * b2g * b3 + b1 * b2gg * b3 + b1 * b2g * b3g +
	    b1g * b2 * b3g + b1 * b2g * b3g + b1 * b2 * b3gg) * coeffs2[1];
    val += (c1gg * c2 * c3 + c1g * c2g * c3 + c1g * c2 * c3g +
	    c1g * c2g * c3 + c1 * c2gg * c3 + c1 * c2g * c3g +
	    c1g * c2 * c3g + c1 * c2g * c3g + c1 * c2 * c3gg) * coeffs2[2];
    val += (a1gg * a2 * b3 + a1gg * b2 * a3 + b1gg * a2 * a3 +
	    a1g * a2g * b3 + a1g * b2g * a3 + b1g * a2g * a3 +
	    a1g * a2 * b3g + a1g * b2 * a3g + b1g * a2 * a3g +
	    a1g * a2g * b3 + a1g * b2g * a3 + b1g * a2g * a3 +
	    a1 * a2gg * b3 + a1 * b2gg * a3 + b1 * a2gg * a3 +
	    a1 * a2g * b3g + a1 * b2g * a3g + b1 * a2g * a3g +
	    a1g * a2 * b3g + a1g * b2 * a3g + b1g * a2 * a3g +
	    a1 * a2g * b3g + a1 * b2g * a3g + b1 * a2g * a3g +
	    a1 * a2 * b3gg + a1 * b2 * a3gg + b1 * a2 * a3gg) * coeffs2[3];
    val += (a1gg * a2 * c3 + a1gg * c2 * a3 + c1gg * a2 * a3 +
	    a1g * a2g * c3 + a1g * c2g * a3 + c1g * a2g * a3 +
	    a1g * a2 * c3g + a1g * c2 * a3g + c1g * a2 * a3g +
	    a1g * a2g * c3 + a1g * c2g * a3 + c1g * a2g * a3 +
	    a1 * a2gg * c3 + a1 * c2gg * a3 + c1 * a2gg * a3 +
	    a1 * a2g * c3g + a1 * c2g * a3g + c1 * a2g * a3g +
	    a1g * a2 * c3g + a1g * c2 * a3g + c1g * a2 * a3g +
	    a1 * a2g * c3g + a1 * c2g * a3g + c1 * a2g * a3g +
	    a1 * a2 * c3gg + a1 * c2 * a3gg + c1 * a2 * a3gg) * coeffs2[4];
    val += (a1gg * b2 * b3 + b1gg * a2 * b3 + b1gg * b2 * a3 +
	    a1g * b2g * b3 + b1g * a2g * b3 + b1g * b2g * a3 +
	    a1g * b2 * b3g + b1g * a2 * b3g + b1g * b2 * a3g +
	    a1g * b2g * b3 + b1g * a2g * b3 + b1g * b2g * a3 +
	    a1 * b2gg * b3 + b1 * a2gg * b3 + b1 * b2gg * a3 +
	    a1 * b2g * b3g + b1 * a2g * b3g + b1 * b2g * a3g +
	    a1g * b2 * b3g + b1g * a2 * b3g + b1g * b2 * a3g +
	    a1 * b2g * b3g + b1 * a2g * b3g + b1 * b2g * a3g +
	    a1 * b2 * b3gg + b1 * a2 * b3gg + b1 * b2 * a3gg) * coeffs2[5];
    val += (b1gg * b2 * c3 + b1gg * c2 * b3 + c1gg * b2 * b3 +
	    b1g * b2g * c3 + b1g * c2g * b3 + c1g * b2g * b3 +
	    b1g * b2 * c3g + b1g * c2 * b3g + c1g * b2 * b3g +
	    b1g * b2g * c3 + b1g * c2g * b3 + c1g * b2g * b3 +
	    b1 * b2gg * c3 + b1 * c2gg * b3 + c1 * b2gg * b3 +
	    b1 * b2g * c3g + b1 * c2g * b3g + c1 * b2g * b3g +
	    b1g * b2 * c3g + b1g * c2 * b3g + c1g * b2 * b3g +
	    b1 * b2g * c3g + b1 * c2g * b3g + c1 * b2g * b3g +
	    b1 * b2 * c3gg + b1 * c2 * b3gg + c1 * b2 * b3gg) * coeffs2[6];
    val += (a1gg * c2 * c3 + c1gg * a2 * c3 + c1gg * c2 * a3 +
	    a1g * c2g * c3 + c1g * a2g * c3 + c1g * c2g * a3 +
	    a1g * c2 * c3g + c1g * a2 * c3g + c1g * c2 * a3g +
	    a1g * c2g * c3 + c1g * a2g * c3 + c1g * c2g * a3 +
	    a1 * c2gg * c3 + c1 * a2gg * c3 + c1 * c2gg * a3 +
	    a1 * c2g * c3g + c1 * a2g * c3g + c1 * c2g * a3g +
	    a1g * c2 * c3g + c1g * a2 * c3g + c1g * c2 * a3g +
	    a1 * c2g * c3g + c1 * a2g * c3g + c1 * c2g * a3g +
	    a1 * c2 * c3gg + c1 * a2 * c3gg + c1 * c2 * a3gg) * coeffs2[7];
    val += (b1gg * c2 * c3 + c1gg * b2 * c3 + c1gg * c2 * b3 +
	    b1g * c2g * c3 + c1g * b2g * c3 + c1g * c2g * b3 +
	    b1g * c2 * c3g + c1g * b2 * c3g + c1g * c2 * b3g +
	    b1g * c2g * c3 + c1g * b2g * c3 + c1g * c2g * b3 +
	    b1 * c2gg * c3 + c1 * b2gg * c3 + c1 * c2gg * b3 +
	    b1 * c2g * c3g + c1 * b2g * c3g + c1 * c2g * b3g +
	    b1g * c2 * c3g + c1g * b2 * c3g + c1g * c2 * b3g +
	    b1 * c2g * c3g + c1 * b2g * c3g + c1 * c2g * b3g +
	    b1 * c2 * c3gg + c1 * b2 * c3gg + c1 * c2 * b3gg) * coeffs2[8];
    val += (a1gg * b2 * c3 + a1gg * c2 * b3 + b1gg * a2 * c3 +
	    a1g * b2g * c3 + a1g * c2g * b3 + b1g * a2g * c3 +
	    a1g * b2 * c3g + a1g * c2 * b3g + b1g * a2 * c3g +
	    a1g * b2g * c3 + a1g * c2g * b3 + b1g * a2g * c3 +
	    a1 * b2gg * c3 + a1 * c2gg * b3 + b1 * a2gg * c3 +
	    a1 * b2g * c3g + a1 * c2g * b3g + b1 * a2g * c3g +
	    a1g * b2 * c3g + a1g * c2 * b3g + b1g * a2 * c3g +
	    a1 * b2g * c3g + a1 * c2g * b3g + b1 * a2g * c3g +
	    a1 * b2 * c3gg + a1 * c2 * b3gg + b1 * a2 * c3gg +
	    b1gg * c2 * a3 + c1gg * a2 * b3 + c1gg * b2 * a3 +
	    b1g * c2g * a3 + c1g * a2g * b3 + c1g * b2g * a3 +
	    b1g * c2 * a3g + c1g * a2 * b3g + c1g * b2 * a3g +
	    b1g * c2g * a3 + c1g * a2g * b3 + c1g * b2g * a3 +
	    b1 * c2gg * a3 + c1 * a2gg * b3 + c1 * b2gg * a3 +
	    b1 * c2g * a3g + c1 * a2g * b3g + c1 * b2g * a3g +
	    b1g * c2 * a3g + c1g * a2 * b3g + c1g * b2 * a3g +
	    b1 * c2g * a3g + c1 * a2g * b3g + c1 * b2g * a3g +
	    b1 * c2 * a3gg + c1 * a2 * b3gg + c1 * b2 * a3gg) * coeffs2[9];
    return (val);
}

static double hesscon01(coeffs2)
double   *coeffs2;
{
    double    val;

    val = (a1tp * a2 * a3 + a1p * a2t * a3 + a1p * a2 * a3t +
	   a1t * a2p * a3 + a1 * a2tp * a3 + a1 * a2p * a3t +
	   a1t * a2 * a3p + a1 * a2t * a3p + a1 * a2 * a3tp) * coeffs2[0];
    val += (b1tp * b2 * b3 + b1p * b2t * b3 + b1p * b2 * b3t +
	    b1t * b2p * b3 + b1 * b2tp * b3 + b1 * b2p * b3t +
	    b1t * b2 * b3p + b1 * b2t * b3p + b1 * b2 * b3tp) * coeffs2[1];
    val += (c1tp * c2 * c3 + c1p * c2t * c3 + c1p * c2 * c3t +
	    c1t * c2p * c3 + c1 * c2tp * c3 + c1 * c2p * c3t +
	    c1t * c2 * c3p + c1 * c2t * c3p + c1 * c2 * c3tp) * coeffs2[2];
    val += (a1tp * a2 * b3 + a1tp * b2 * a3 + b1tp * a2 * a3 +
	    a1t * a2p * b3 + a1t * b2p * a3 + b1t * a2p * a3 +
	    a1t * a2 * b3p + a1t * b2 * a3p + b1t * a2 * a3p +
	    a1p * a2t * b3 + a1p * b2t * a3 + b1p * a2t * a3 +
	    a1 * a2tp * b3 + a1 * b2tp * a3 + b1 * a2tp * a3 +
	    a1 * a2t * b3p + a1 * b2t * a3p + b1 * a2t * a3p +
	    a1p * a2 * b3t + a1p * b2 * a3t + b1p * a2 * a3t +
	    a1 * a2p * b3t + a1 * b2p * a3t + b1 * a2p * a3t +
	    a1 * a2 * b3tp + a1 * b2 * a3tp + b1 * a2 * a3tp) * coeffs2[3];
    val += (a1tp * a2 * c3 + a1tp * c2 * a3 + c1tp * a2 * a3 +
	    a1t * a2p * c3 + a1t * c2p * a3 + c1t * a2p * a3 +
	    a1t * a2 * c3p + a1t * c2 * a3p + c1t * a2 * a3p +
	    a1p * a2t * c3 + a1p * c2t * a3 + c1p * a2t * a3 +
	    a1 * a2tp * c3 + a1 * c2tp * a3 + c1 * a2tp * a3 +
	    a1 * a2t * c3p + a1 * c2t * a3p + c1 * a2t * a3p +
	    a1p * a2 * c3t + a1p * c2 * a3t + c1p * a2 * a3t +
	    a1 * a2p * c3t + a1 * c2p * a3t + c1 * a2p * a3t +
	    a1 * a2 * c3tp + a1 * c2 * a3tp + c1 * a2 * a3tp) * coeffs2[4];
    val += (a1tp * b2 * b3 + b1tp * a2 * b3 + b1tp * b2 * a3 +
	    a1t * b2p * b3 + b1t * a2p * b3 + b1t * b2p * a3 +
	    a1t * b2 * b3p + b1t * a2 * b3p + b1t * b2 * a3p +
	    a1p * b2t * b3 + b1p * a2t * b3 + b1p * b2t * a3 +
	    a1 * b2tp * b3 + b1 * a2tp * b3 + b1 * b2tp * a3 +
	    a1 * b2t * b3p + b1 * a2t * b3p + b1 * b2t * a3p +
	    a1p * b2 * b3t + b1p * a2 * b3t + b1p * b2 * a3t +
	    a1 * b2p * b3t + b1 * a2p * b3t + b1 * b2p * a3t +
	    a1 * b2 * b3tp + b1 * a2 * b3tp + b1 * b2 * a3tp) * coeffs2[5];
    val += (b1tp * b2 * c3 + b1tp * c2 * b3 + c1tp * b2 * b3 +
	    b1t * b2p * c3 + b1t * c2p * b3 + c1t * b2p * b3 +
	    b1t * b2 * c3p + b1t * c2 * b3p + c1t * b2 * b3p +
	    b1p * b2t * c3 + b1p * c2t * b3 + c1p * b2t * b3 +
	    b1 * b2tp * c3 + b1 * c2tp * b3 + c1 * b2tp * b3 +
	    b1 * b2t * c3p + b1 * c2t * b3p + c1 * b2t * b3p +
	    b1p * b2 * c3t + b1p * c2 * b3t + c1p * b2 * b3t +
	    b1 * b2p * c3t + b1 * c2p * b3t + c1 * b2p * b3t +
	    b1 * b2 * c3tp + b1 * c2 * b3tp + c1 * b2 * b3tp) * coeffs2[6];
    val += (a1tp * c2 * c3 + c1tp * a2 * c3 + c1tp * c2 * a3 +
	    a1t * c2p * c3 + c1t * a2p * c3 + c1t * c2p * a3 +
	    a1t * c2 * c3p + c1t * a2 * c3p + c1t * c2 * a3p +
	    a1p * c2t * c3 + c1p * a2t * c3 + c1p * c2t * a3 +
	    a1 * c2tp * c3 + c1 * a2tp * c3 + c1 * c2tp * a3 +
	    a1 * c2t * c3p + c1 * a2t * c3p + c1 * c2t * a3p +
	    a1p * c2 * c3t + c1p * a2 * c3t + c1p * c2 * a3t +
	    a1 * c2p * c3t + c1 * a2p * c3t + c1 * c2p * a3t +
	    a1 * c2 * c3tp + c1 * a2 * c3tp + c1 * c2 * a3tp) * coeffs2[7];
    val += (b1tp * c2 * c3 + c1tp * b2 * c3 + c1tp * c2 * b3 +
	    b1t * c2p * c3 + c1t * b2p * c3 + c1t * c2p * b3 +
	    b1t * c2 * c3p + c1t * b2 * c3p + c1t * c2 * b3p +
	    b1p * c2t * c3 + c1p * b2t * c3 + c1p * c2t * b3 +
	    b1 * c2tp * c3 + c1 * b2tp * c3 + c1 * c2tp * b3 +
	    b1 * c2t * c3p + c1 * b2t * c3p + c1 * c2t * b3p +
	    b1p * c2 * c3t + c1p * b2 * c3t + c1p * c2 * b3t +
	    b1 * c2p * c3t + c1 * b2p * c3t + c1 * c2p * b3t +
	    b1 * c2 * c3tp + c1 * b2 * c3tp + c1 * c2 * b3tp) * coeffs2[8];
    val += (a1tp * b2 * c3 + a1tp * c2 * b3 + b1tp * a2 * c3 +
	    a1t * b2p * c3 + a1t * c2p * b3 + b1t * a2p * c3 +
	    a1t * b2 * c3p + a1t * c2 * b3p + b1t * a2 * c3p +
	    a1p * b2t * c3 + a1p * c2t * b3 + b1p * a2t * c3 +
	    a1 * b2tp * c3 + a1 * c2tp * b3 + b1 * a2tp * c3 +
	    a1 * b2t * c3p + a1 * c2t * b3p + b1 * a2t * c3p +
	    a1p * b2 * c3t + a1p * c2 * b3t + b1p * a2 * c3t +
	    a1 * b2p * c3t + a1 * c2p * b3t + b1 * a2p * c3t +
	    a1 * b2 * c3tp + a1 * c2 * b3tp + b1 * a2 * c3tp +
	    b1tp * c2 * a3 + c1tp * a2 * b3 + c1tp * b2 * a3 +
	    b1t * c2p * a3 + c1t * a2p * b3 + c1t * b2p * a3 +
	    b1t * c2 * a3p + c1t * a2 * b3p + c1t * b2 * a3p +
	    b1p * c2t * a3 + c1p * a2t * b3 + c1p * b2t * a3 +
	    b1 * c2tp * a3 + c1 * a2tp * b3 + c1 * b2tp * a3 +
	    b1 * c2t * a3p + c1 * a2t * b3p + c1 * b2t * a3p +
	    b1p * c2 * a3t + c1p * a2 * b3t + c1p * b2 * a3t +
	    b1 * c2p * a3t + c1 * a2p * b3t + c1 * b2p * a3t +
	    b1 * c2 * a3tp + c1 * a2 * b3tp + c1 * b2 * a3tp) * coeffs2[9];
    return (val);
}

static double hesscon02(coeffs2)
double   *coeffs2;
{
    double    val;

    val = (a1tg * a2 * a3 + a1g * a2t * a3 + a1g * a2 * a3t +
	   a1t * a2g * a3 + a1 * a2tg * a3 + a1 * a2g * a3t +
	   a1t * a2 * a3g + a1 * a2t * a3g + a1 * a2 * a3tg) * coeffs2[0];
    val += (b1tg * b2 * b3 + b1g * b2t * b3 + b1g * b2 * b3t +
	    b1t * b2g * b3 + b1 * b2tg * b3 + b1 * b2g * b3t +
	    b1t * b2 * b3g + b1 * b2t * b3g + b1 * b2 * b3tg) * coeffs2[1];
    val += (c1tg * c2 * c3 + c1g * c2t * c3 + c1g * c2 * c3t +
	    c1t * c2g * c3 + c1 * c2tg * c3 + c1 * c2g * c3t +
	    c1t * c2 * c3g + c1 * c2t * c3g + c1 * c2 * c3tg) * coeffs2[2];
    val += (a1tg * a2 * b3 + a1tg * b2 * a3 + b1tg * a2 * a3 +
	    a1t * a2g * b3 + a1t * b2g * a3 + b1t * a2g * a3 +
	    a1t * a2 * b3g + a1t * b2 * a3g + b1t * a2 * a3g +
	    a1g * a2t * b3 + a1g * b2t * a3 + b1g * a2t * a3 +
	    a1 * a2tg * b3 + a1 * b2tg * a3 + b1 * a2tg * a3 +
	    a1 * a2t * b3g + a1 * b2t * a3g + b1 * a2t * a3g +
	    a1g * a2 * b3t + a1g * b2 * a3t + b1g * a2 * a3t +
	    a1 * a2g * b3t + a1 * b2g * a3t + b1 * a2g * a3t +
	    a1 * a2 * b3tg + a1 * b2 * a3tg + b1 * a2 * a3tg) * coeffs2[3];
    val += (a1tg * a2 * c3 + a1tg * c2 * a3 + c1tg * a2 * a3 +
	    a1t * a2g * c3 + a1t * c2g * a3 + c1t * a2g * a3 +
	    a1t * a2 * c3g + a1t * c2 * a3g + c1t * a2 * a3g +
	    a1g * a2t * c3 + a1g * c2t * a3 + c1g * a2t * a3 +
	    a1 * a2tg * c3 + a1 * c2tg * a3 + c1 * a2tg * a3 +
	    a1 * a2t * c3g + a1 * c2t * a3g + c1 * a2t * a3g +
	    a1g * a2 * c3t + a1g * c2 * a3t + c1g * a2 * a3t +
	    a1 * a2g * c3t + a1 * c2g * a3t + c1 * a2g * a3t +
	    a1 * a2 * c3tg + a1 * c2 * a3tg + c1 * a2 * a3tg) * coeffs2[4];
    val += (a1tg * b2 * b3 + b1tg * a2 * b3 + b1tg * b2 * a3 +
	    a1t * b2g * b3 + b1t * a2g * b3 + b1t * b2g * a3 +
	    a1t * b2 * b3g + b1t * a2 * b3g + b1t * b2 * a3g +
	    a1g * b2t * b3 + b1g * a2t * b3 + b1g * b2t * a3 +
	    a1 * b2tg * b3 + b1 * a2tg * b3 + b1 * b2tg * a3 +
	    a1 * b2t * b3g + b1 * a2t * b3g + b1 * b2t * a3g +
	    a1g * b2 * b3t + b1g * a2 * b3t + b1g * b2 * a3t +
	    a1 * b2g * b3t + b1 * a2g * b3t + b1 * b2g * a3t +
	    a1 * b2 * b3tg + b1 * a2 * b3tg + b1 * b2 * a3tg) * coeffs2[5];
    val += (b1tg * b2 * c3 + b1tg * c2 * b3 + c1tg * b2 * b3 +
	    b1t * b2g * c3 + b1t * c2g * b3 + c1t * b2g * b3 +
	    b1t * b2 * c3g + b1t * c2 * b3g + c1t * b2 * b3g +
	    b1g * b2t * c3 + b1g * c2t * b3 + c1g * b2t * b3 +
	    b1 * b2tg * c3 + b1 * c2tg * b3 + c1 * b2tg * b3 +
	    b1 * b2t * c3g + b1 * c2t * b3g + c1 * b2t * b3g +
	    b1g * b2 * c3t + b1g * c2 * b3t + c1g * b2 * b3t +
	    b1 * b2g * c3t + b1 * c2g * b3t + c1 * b2g * b3t +
	    b1 * b2 * c3tg + b1 * c2 * b3tg + c1 * b2 * b3tg) * coeffs2[6];
    val += (a1tg * c2 * c3 + c1tg * a2 * c3 + c1tg * c2 * a3 +
	    a1t * c2g * c3 + c1t * a2g * c3 + c1t * c2g * a3 +
	    a1t * c2 * c3g + c1t * a2 * c3g + c1t * c2 * a3g +
	    a1g * c2t * c3 + c1g * a2t * c3 + c1g * c2t * a3 +
	    a1 * c2tg * c3 + c1 * a2tg * c3 + c1 * c2tg * a3 +
	    a1 * c2t * c3g + c1 * a2t * c3g + c1 * c2t * a3g +
	    a1g * c2 * c3t + c1g * a2 * c3t + c1g * c2 * a3t +
	    a1 * c2g * c3t + c1 * a2g * c3t + c1 * c2g * a3t +
	    a1 * c2 * c3tg + c1 * a2 * c3tg + c1 * c2 * a3tg) * coeffs2[7];
    val += (b1tg * c2 * c3 + c1tg * b2 * c3 + c1tg * c2 * b3 +
	    b1t * c2g * c3 + c1t * b2g * c3 + c1t * c2g * b3 +
	    b1t * c2 * c3g + c1t * b2 * c3g + c1t * c2 * b3g +
	    b1g * c2t * c3 + c1g * b2t * c3 + c1g * c2t * b3 +
	    b1 * c2tg * c3 + c1 * b2tg * c3 + c1 * c2tg * b3 +
	    b1 * c2t * c3g + c1 * b2t * c3g + c1 * c2t * b3g +
	    b1g * c2 * c3t + c1g * b2 * c3t + c1g * c2 * b3t +
	    b1 * c2g * c3t + c1 * b2g * c3t + c1 * c2g * b3t +
	    b1 * c2 * c3tg + c1 * b2 * c3tg + c1 * c2 * b3tg) * coeffs2[8];
    val += (a1tg * b2 * c3 + a1tg * c2 * b3 + b1tg * a2 * c3 +
	    a1t * b2g * c3 + a1t * c2g * b3 + b1t * a2g * c3 +
	    a1t * b2 * c3g + a1t * c2 * b3g + b1t * a2 * c3g +
	    a1g * b2t * c3 + a1g * c2t * b3 + b1g * a2t * c3 +
	    a1 * b2tg * c3 + a1 * c2tg * b3 + b1 * a2tg * c3 +
	    a1 * b2t * c3g + a1 * c2t * b3g + b1 * a2t * c3g +
	    a1g * b2 * c3t + a1g * c2 * b3t + b1g * a2 * c3t +
	    a1 * b2g * c3t + a1 * c2g * b3t + b1 * a2g * c3t +
	    a1 * b2 * c3tg + a1 * c2 * b3tg + b1 * a2 * c3tg +
	    b1tg * c2 * a3 + c1tg * a2 * b3 + c1tg * b2 * a3 +
	    b1t * c2g * a3 + c1t * a2g * b3 + c1t * b2g * a3 +
	    b1t * c2 * a3g + c1t * a2 * b3g + c1t * b2 * a3g +
	    b1g * c2t * a3 + c1g * a2t * b3 + c1g * b2t * a3 +
	    b1 * c2tg * a3 + c1 * a2tg * b3 + c1 * b2tg * a3 +
	    b1 * c2t * a3g + c1 * a2t * b3g + c1 * b2t * a3g +
	    b1g * c2 * a3t + c1g * a2 * b3t + c1g * b2 * a3t +
	    b1 * c2g * a3t + c1 * a2g * b3t + c1 * b2g * a3t +
	    b1 * c2 * a3tg + c1 * a2 * b3tg + c1 * b2 * a3tg) * coeffs2[9];
    return (val);
}

static double hesscon12(coeffs2)
double   *coeffs2;
{
    double    val;

    val = (a1pg * a2 * a3 + a1g * a2p * a3 + a1g * a2 * a3p +
	   a1p * a2g * a3 + a1 * a2pg * a3 + a1 * a2g * a3p +
	   a1p * a2 * a3g + a1 * a2p * a3g + a1 * a2 * a3pg) * coeffs2[0];
    val += (b1pg * b2 * b3 + b1g * b2p * b3 + b1g * b2 * b3p +
	    b1p * b2g * b3 + b1 * b2pg * b3 + b1 * b2g * b3p +
	    b1p * b2 * b3g + b1 * b2p * b3g + b1 * b2 * b3pg) * coeffs2[1];
    val += (c1pg * c2 * c3 + c1g * c2p * c3 + c1g * c2 * c3p +
	    c1p * c2g * c3 + c1 * c2pg * c3 + c1 * c2g * c3p +
	    c1p * c2 * c3g + c1 * c2p * c3g + c1 * c2 * c3pg) * coeffs2[2];
    val += (a1pg * a2 * b3 + a1pg * b2 * a3 + b1pg * a2 * a3 +
	    a1p * a2g * b3 + a1p * b2g * a3 + b1p * a2g * a3 +
	    a1p * a2 * b3g + a1p * b2 * a3g + b1p * a2 * a3g +
	    a1g * a2p * b3 + a1g * b2p * a3 + b1g * a2p * a3 +
	    a1 * a2pg * b3 + a1 * b2pg * a3 + b1 * a2pg * a3 +
	    a1 * a2p * b3g + a1 * b2p * a3g + b1 * a2p * a3g +
	    a1g * a2 * b3p + a1g * b2 * a3p + b1g * a2 * a3p +
	    a1 * a2g * b3p + a1 * b2g * a3p + b1 * a2g * a3p +
	    a1 * a2 * b3pg + a1 * b2 * a3pg + b1 * a2 * a3pg) * coeffs2[3];
    val += (a1pg * a2 * c3 + a1pg * c2 * a3 + c1pg * a2 * a3 +
	    a1p * a2g * c3 + a1p * c2g * a3 + c1p * a2g * a3 +
	    a1p * a2 * c3g + a1p * c2 * a3g + c1p * a2 * a3g +
	    a1g * a2p * c3 + a1g * c2p * a3 + c1g * a2p * a3 +
	    a1 * a2pg * c3 + a1 * c2pg * a3 + c1 * a2pg * a3 +
	    a1 * a2p * c3g + a1 * c2p * a3g + c1 * a2p * a3g +
	    a1g * a2 * c3p + a1g * c2 * a3p + c1g * a2 * a3p +
	    a1 * a2g * c3p + a1 * c2g * a3p + c1 * a2g * a3p +
	    a1 * a2 * c3pg + a1 * c2 * a3pg + c1 * a2 * a3pg) * coeffs2[4];
    val += (a1pg * b2 * b3 + b1pg * a2 * b3 + b1pg * b2 * a3 +
	    a1p * b2g * b3 + b1p * a2g * b3 + b1p * b2g * a3 +
	    a1p * b2 * b3g + b1p * a2 * b3g + b1p * b2 * a3g +
	    a1g * b2p * b3 + b1g * a2p * b3 + b1g * b2p * a3 +
	    a1 * b2pg * b3 + b1 * a2pg * b3 + b1 * b2pg * a3 +
	    a1 * b2p * b3g + b1 * a2p * b3g + b1 * b2p * a3g +
	    a1g * b2 * b3p + b1g * a2 * b3p + b1g * b2 * a3p +
	    a1 * b2g * b3p + b1 * a2g * b3p + b1 * b2g * a3p +
	    a1 * b2 * b3pg + b1 * a2 * b3pg + b1 * b2 * a3pg) * coeffs2[5];
    val += (b1pg * b2 * c3 + b1pg * c2 * b3 + c1pg * b2 * b3 +
	    b1p * b2g * c3 + b1p * c2g * b3 + c1p * b2g * b3 +
	    b1p * b2 * c3g + b1p * c2 * b3g + c1p * b2 * b3g +
	    b1g * b2p * c3 + b1g * c2p * b3 + c1g * b2p * b3 +
	    b1 * b2pg * c3 + b1 * c2pg * b3 + c1 * b2pg * b3 +
	    b1 * b2p * c3g + b1 * c2p * b3g + c1 * b2p * b3g +
	    b1g * b2 * c3p + b1g * c2 * b3p + c1g * b2 * b3p +
	    b1 * b2g * c3p + b1 * c2g * b3p + c1 * b2g * b3p +
	    b1 * b2 * c3pg + b1 * c2 * b3pg + c1 * b2 * b3pg) * coeffs2[6];
    val += (a1pg * c2 * c3 + c1pg * a2 * c3 + c1pg * c2 * a3 +
	    a1p * c2g * c3 + c1p * a2g * c3 + c1p * c2g * a3 +
	    a1p * c2 * c3g + c1p * a2 * c3g + c1p * c2 * a3g +
	    a1g * c2p * c3 + c1g * a2p * c3 + c1g * c2p * a3 +
	    a1 * c2pg * c3 + c1 * a2pg * c3 + c1 * c2pg * a3 +
	    a1 * c2p * c3g + c1 * a2p * c3g + c1 * c2p * a3g +
	    a1g * c2 * c3p + c1g * a2 * c3p + c1g * c2 * a3p +
	    a1 * c2g * c3p + c1 * a2g * c3p + c1 * c2g * a3p +
	    a1 * c2 * c3pg + c1 * a2 * c3pg + c1 * c2 * a3pg) * coeffs2[7];
    val += (b1pg * c2 * c3 + c1pg * b2 * c3 + c1pg * c2 * b3 +
	    b1p * c2g * c3 + c1p * b2g * c3 + c1p * c2g * b3 +
	    b1p * c2 * c3g + c1p * b2 * c3g + c1p * c2 * b3g +
	    b1g * c2p * c3 + c1g * b2p * c3 + c1g * c2p * b3 +
	    b1 * c2pg * c3 + c1 * b2pg * c3 + c1 * c2pg * b3 +
	    b1 * c2p * c3g + c1 * b2p * c3g + c1 * c2p * b3g +
	    b1g * c2 * c3p + c1g * b2 * c3p + c1g * c2 * b3p +
	    b1 * c2g * c3p + c1 * b2g * c3p + c1 * c2g * b3p +
	    b1 * c2 * c3pg + c1 * b2 * c3pg + c1 * c2 * b3pg) * coeffs2[8];
    val += (a1pg * b2 * c3 + a1pg * c2 * b3 + b1pg * a2 * c3 +
	    a1p * b2g * c3 + a1p * c2g * b3 + b1p * a2g * c3 +
	    a1p * b2 * c3g + a1p * c2 * b3g + b1p * a2 * c3g +
	    a1g * b2p * c3 + a1g * c2p * b3 + b1g * a2p * c3 +
	    a1 * b2pg * c3 + a1 * c2pg * b3 + b1 * a2pg * c3 +
	    a1 * b2p * c3g + a1 * c2p * b3g + b1 * a2p * c3g +
	    a1g * b2 * c3p + a1g * c2 * b3p + b1g * a2 * c3p +
	    a1 * b2g * c3p + a1 * c2g * b3p + b1 * a2g * c3p +
	    a1 * b2 * c3pg + a1 * c2 * b3pg + b1 * a2 * c3pg +
	    b1pg * c2 * a3 + c1pg * a2 * b3 + c1pg * b2 * a3 +
	    b1p * c2g * a3 + c1p * a2g * b3 + c1p * b2g * a3 +
	    b1p * c2 * a3g + c1p * a2 * b3g + c1p * b2 * a3g +
	    b1g * c2p * a3 + c1g * a2p * b3 + c1g * b2p * a3 +
	    b1 * c2pg * a3 + c1 * a2pg * b3 + c1 * b2pg * a3 +
	    b1 * c2p * a3g + c1 * a2p * b3g + c1 * b2p * a3g +
	    b1g * c2 * a3p + c1g * a2 * b3p + c1g * b2 * a3p +
	    b1 * c2g * a3p + c1 * a2g * b3p + c1 * b2g * a3p +
	    b1 * c2 * a3pg + c1 * a2 * b3pg + c1 * b2 * a3pg) * coeffs2[9];
    return (val);
}
