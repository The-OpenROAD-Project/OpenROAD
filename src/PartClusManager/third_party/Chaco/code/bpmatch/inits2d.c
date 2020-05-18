/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"params.h"
#include	"structs.h"
#include	"defs.h"


void      inits2d(graph, xvecs, vals, indices, nvtxs, dist, startvtx, size, sets)
struct vtx_data **graph;	/* graph data structure for vertex weights */
double  **xvecs;		/* values to partition with */
double   *vals[4][MAXSETS];	/* values in sorted lists */
int      *indices[4][MAXSETS];	/* indices sorting lists */
int       nvtxs;		/* number of vertices */
double   *dist;			/* trial separation point */
int       startvtx[4][MAXSETS];	/* indices defining separation */
double   *size;			/* size of each set being modified */
short    *sets;			/* set each vertex gets assigned to */
{
    double    xmid, ymid;	/* median x and y values */
    double    val, bestval;	/* values for determining set preferences */
    short     bestset;		/* set vertex wants to be in */
    int       signx, signy;	/* sign values for different target points */
    int       nsets = 4;	/* number of different sets */
    int       i, j;		/* loop counters */
    int       findindex();

/*
    xmid = .25 * (vals[0][1][indices[0][1][nvtxs / 2]] +
		  vals[0][1][indices[0][1][nvtxs / 2 - 1]]);
    ymid = .25 * (vals[0][2][indices[0][2][nvtxs / 2]] +
		  vals[0][2][indices[0][2][nvtxs / 2 - 1]]);
*/
    xmid = .5 * vals[0][1][indices[0][1][nvtxs / 2]];
    ymid = .5 * vals[0][2][indices[0][2][nvtxs / 2]];

    dist[0] = -xmid - ymid;
    dist[1] = xmid - ymid;
    dist[2] = -xmid + ymid;
    dist[3] = xmid + ymid;

    /* Now initialize startvtx. */
    startvtx[0][1] = startvtx[2][3] = nvtxs / 2;
    startvtx[0][2] = startvtx[1][3] = nvtxs / 2;
    startvtx[1][2] = findindex(indices[1][2], vals[1][2], dist[2] - dist[1], nvtxs);
    startvtx[0][3] = findindex(indices[0][3], vals[0][3], dist[3] - dist[0], nvtxs);

    for (i = 0; i < nsets; i++)
	size[i] = 0;

    for (i = 1; i <= nvtxs; i++) {
	/* Which set is this vertex in? */
	signx = signy = -1;
	bestval = 0;
	for (j = 0; j < nsets; j++) {
	    val = -dist[j] + 2 * (signx * xvecs[1][i] + signy * xvecs[2][i]);
	    if (j == 0 || val < bestval) {
		bestval = val;
		bestset = (short) j;
	    }
	    if (signx == 1)
		signy *= -1;
	    signx *= -1;
	}
	sets[i] = bestset;
	size[bestset] += graph[i]->vwgt;
    }
}


int       findindex(indices, vals, target, nvals)
int      *indices;		/* indices sorting values */
double   *vals;			/* values sorted by indices */
double    target;		/* target value */
int       nvals;		/* number of values */
{
    double    ratio;		/* interpolation parameter */
    double    vlow, vhigh;	/* values at limits of search range */
    int       low, high;	/* range left to search */
    int       new;		/* new index limit */

    if (target <= vals[indices[0]])
	return (0);
    if (target >= vals[indices[nvals - 1]])
	return (nvals - 1);

    low = 0;
    high = nvals - 1;

    while (high - low > 1) {
	vlow = vals[indices[low]];
	vhigh = vals[indices[high]];
	if (vlow == vhigh)
	    return ((vlow + vhigh) / 2);

	ratio = (target - vlow) / (vhigh - vlow);
	new = low + ratio * (high - low);
	if (new == low)
	    ++new;
	else if (new == high)
	    --new;

	if (vals[indices[new]] < target)
	    low = new;
	else
	    high = new;
    }

    if (target == vals[indices[high]])
	return (high);
    else
	return (low);
}
