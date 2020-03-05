/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"defs.h"
#include	"params.h"


void      genvals3d(xvecs, vals, nvtxs)
/* Create lists of sets of values to be sorted. */
double  **xvecs;		/* vectors to partition */
double   *vals[8][MAXSETS];	/* ptrs to lists of values */
int       nvtxs;		/* number of values */
{
    int       nsets = 8;	/* number of sets */
    int       nlists = 13;	/* number of lists to generate */
    double   *temp[13];		/* place holders for vals */
    int       i, j;		/* loop counter */
    double   *smalloc();

    for (i = 0; i < nlists; i++) {
	temp[i] = (double *) smalloc((unsigned) nvtxs * sizeof(double));
    }

    for (i = 1; i <= nvtxs; i++) {
	temp[0][i - 1] = 4 * xvecs[1][i];
	temp[1][i - 1] = 4 * xvecs[2][i];
	temp[2][i - 1] = 4 * xvecs[3][i];
	temp[3][i - 1] = 4 * (xvecs[1][i] + xvecs[2][i]);
	temp[4][i - 1] = 4 * (-xvecs[1][i] + xvecs[2][i]);
	temp[5][i - 1] = 4 * (xvecs[1][i] + xvecs[3][i]);
	temp[6][i - 1] = 4 * (-xvecs[1][i] + xvecs[3][i]);
	temp[7][i - 1] = 4 * (xvecs[2][i] + xvecs[3][i]);
	temp[8][i - 1] = 4 * (-xvecs[2][i] + xvecs[3][i]);
	temp[9][i - 1] = 4 * (xvecs[1][i] + xvecs[2][i] + xvecs[3][i]);
	temp[10][i - 1] = 4 * (-xvecs[1][i] + xvecs[2][i] + xvecs[3][i]);
	temp[11][i - 1] = 4 * (xvecs[1][i] - xvecs[2][i] + xvecs[3][i]);
	temp[12][i - 1] = 4 * (-xvecs[1][i] - xvecs[2][i] + xvecs[3][i]);
    }

    vals[0][1] = vals[2][3] = vals[4][5] = vals[6][7] = temp[0];
    vals[0][2] = vals[1][3] = vals[4][6] = vals[5][7] = temp[1];
    vals[0][4] = vals[1][5] = vals[2][6] = vals[3][7] = temp[2];
    vals[0][3] = vals[4][7] = temp[3];
    vals[1][2] = vals[5][6] = temp[4];
    vals[0][5] = vals[2][7] = temp[5];
    vals[1][4] = vals[3][6] = temp[6];
    vals[0][6] = vals[1][7] = temp[7];
    vals[2][4] = vals[3][5] = temp[8];
    vals[0][7] = temp[9];
    vals[1][6] = temp[10];
    vals[2][5] = temp[11];
    vals[3][4] = temp[12];

    for (i = 0; i < nsets; i++) {
	for (j = i + 1; j < nsets; j++)
	    vals[j][i] = vals[i][j];
    }
}
