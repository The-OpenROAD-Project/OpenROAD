/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	<math.h>
#include	"defs.h"


void      rotate2d(yvecs, nmyvtxs, theta)
double  **yvecs;		/* ptr to list of y-vectors (rotated) */
int       nmyvtxs;		/* length of yvecs */
double    theta;		/* angle to rotate by */
{
    double    temp1;		/* hold values for a while */
    double    c, s;		/* cosine and sine of theta */
    int       i;		/* loop counter */

    s = sin(theta);
    c = cos(theta);

    for (i = 1; i <= nmyvtxs; i++) {
	temp1 = yvecs[1][i];
	yvecs[1][i] = c * temp1 + s * yvecs[2][i];
	yvecs[2][i] = -s * temp1 + c * yvecs[2][i];
    }
}


void      rotate3d(yvecs, nmyvtxs, theta, phi, gamma2)
double  **yvecs;		/* ptr to list of y-vectors (to be rotated) */
int       nmyvtxs;		/* length of yvecs */
double    theta, phi, gamma2;	/* rotational parameters */
{
    double    temp1, temp2;	/* hold values for a while */
    double    ctheta, stheta;	/* cosine and sine of theta */
    double    cphi, sphi;	/* cosine and sine of phi */
    double    cgamma, sgamma;	/* cosine and sine of gamma */
    double    onemcg;		/* 1.0 - cosine(gamma) */
    double    a1, a2, a3;	/* rotation matrix entries */
    double    b1, b2, b3;	/* rotation matrix entries */
    double    c1, c2, c3;	/* rotation matrix entries */
    int       i;		/* loop counter */

    stheta = sin(theta);
    ctheta = cos(theta);
    sphi = sin(phi);
    cphi = cos(phi);
    sgamma = sin(gamma2);
    cgamma = cos(gamma2);

    onemcg = 1.0 - cgamma;

    a1 = cgamma + cphi * ctheta * onemcg * cphi * ctheta;
    a2 = sgamma * sphi + cphi * stheta * onemcg * cphi * ctheta;
    a3 = -sgamma * cphi * stheta + sphi * onemcg * cphi * ctheta;

    b1 = -sgamma * sphi + cphi * ctheta * onemcg * cphi * stheta;
    b2 = cgamma + cphi * stheta * onemcg * cphi * stheta;
    b3 = sgamma * cphi * ctheta + sphi * onemcg * cphi * stheta;

    c1 = sgamma * cphi * stheta + cphi * ctheta * onemcg * sphi;
    c2 = -sgamma * cphi * ctheta + cphi * stheta * onemcg * sphi;
    c3 = cgamma + sphi * onemcg * sphi;

    for (i = 1; i <= nmyvtxs; i++) {
	temp1 = yvecs[1][i];
	temp2 = yvecs[2][i];

	yvecs[1][i] = a1 * temp1 + b1 * temp2 + c1 * yvecs[3][i];
	yvecs[2][i] = a2 * temp1 + b2 * temp2 + c2 * yvecs[3][i];
	yvecs[3][i] = a3 * temp1 + b3 * temp2 + c3 * yvecs[3][i];
    }
}
