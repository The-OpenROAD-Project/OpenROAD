/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<math.h>

static double s, c;		/* sign and cosine of angle */
static double s2, cos2;		/* squares of sign and cosine of angle */


double    func2d(coeffs, theta)
double    coeffs[5];		/* five different 4-way products */
double    theta;		/* angular parameter */

/* Returns value of penalty function at given angle. */
{
    double    val;		/* functional value */

    s = sin(theta);
    c = cos(theta);
    cos2 = c * c;
    s2 = s * s;

    val = (cos2 * cos2 + s2 * s2) * (coeffs[0] + coeffs[4]);
    val += 12 * cos2 * s2 * coeffs[2];
    val += 4 * (s2 * s * c - cos2 * c * s) * (coeffs[3] - coeffs[1]);

    return (val);
}


double    grad2d(coeffs, theta)
double    coeffs[5];		/* five different 4-way products */
double    theta;		/* angular parameter */

/* Returns 1st derivative of penalty function at given angle. */
{
    double    val;		/* functional value */

    s = sin(theta);
    c = cos(theta);
    cos2 = c * c;
    s2 = s * s;

    val = 4 * (cos2 * cos2 + s2 * s2) * (coeffs[1] - coeffs[3]);
    val += 24 * cos2 * s2 * (coeffs[3] - coeffs[1]);
    val += 4 * (s2 * s * c - cos2 * c * s) * (coeffs[0] + coeffs[4] - 6 * coeffs[2]);

    return (val);
}


double    hess2d(coeffs)
double    coeffs[5];		/* five different 4-way products */

/* Returns 2nd derivative of penalty function at given angle. */
{
    double    val;		/* functional value */

    val = -4 * (cos2 * cos2 + s2 * s2) * (coeffs[0] + coeffs[4] - 6 * coeffs[2]);
    val += 24 * s2 * cos2 * (coeffs[0] + coeffs[4] - 6 * coeffs[2]);
    val += 64 * (s2 * s * c - cos2 * c * s) * (coeffs[1] - coeffs[3]);

    return (val);
}
