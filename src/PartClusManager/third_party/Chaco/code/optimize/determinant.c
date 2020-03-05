/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */


double    determinant(M, ndims)
double    M[3][3];
int       ndims;
{
    if (ndims == 1)
	return (M[0][0]);

    else if (ndims == 2) {
	return (M[0][0] * M[1][1] - M[0][1] * M[1][0]);
    }

    else if (ndims == 3) {
	return (M[0][0] * (M[1][1] * M[2][2] - M[2][1] * M[1][2])
		- M[1][0] * (M[0][1] * M[2][2] - M[2][1] * M[0][2])
		+ M[2][0] * (M[0][1] * M[1][2] - M[1][1] * M[0][2]));
    }

    else
	return (0.0);
}
