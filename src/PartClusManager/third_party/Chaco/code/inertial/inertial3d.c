/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <math.h>
#include <stdio.h>
#include "structs.h"
#include "defs.h"


void      inertial3d(graph, nvtxs, cube_or_mesh, nsets, x, y, z, sets, goal,
		               using_vwgts)
struct vtx_data **graph;	/* graph data structure */
int       nvtxs;		/* number of vtxs in graph */
int       cube_or_mesh;		/* 0 => hypercube, d => d-dimensional mesh */
int       nsets;		/* number of sets to divide into */
float    *x, *y, *z;		/* x, y and z coordinates of vertices */
short    *sets;			/* set each vertex gets assigned to */
double   *goal;			/* desired set sizes */
int       using_vwgts;		/* are vertex weights being used? */
{
    extern int DEBUG_INERTIAL;	/* debug flag for inertial method */
    extern double inertial_axis_time;	/* time spent computing inertial axis */
    extern double median_time;	/* time spent finding medians */
    double    tensor[3][3];	/* inertia tensor */
    double    evec[3];		/* eigenvector */
    double   *value;		/* values along selected direction to sort */
    double    xcm, ycm, zcm;	/* center of mass in each direction */
    double    xx, yy, zz;	/* elements of inertial tensor */
    double    xy, xz, yz;	/* elements of inertial tensor */
    double    xdif, ydif;	/* deviation from center of mass */
    double    zdif;		/* deviation from center of mass */
    double    eval, res;	/* eigenvalue and error in eval calculation */
    double    vwgt_sum;		/* sum of all the vertex weights */
    double    time;		/* timing parameter */
    int      *space;		/* space required by median routine */
    int       i;		/* loop counter */
    double   *smalloc(), seconds();
    int       sfree();
    void      eigenvec3(), evals3(), rec_median_1();

    /* Compute center of mass and total mass. */
    time = seconds();
    xcm = ycm = zcm = 0.0;
    if (using_vwgts) {
	vwgt_sum = 0;
	for (i = 1; i <= nvtxs; i++) {
	    vwgt_sum += graph[i]->vwgt;
	    xcm += graph[i]->vwgt * x[i];
	    ycm += graph[i]->vwgt * y[i];
	    zcm += graph[i]->vwgt * z[i];
	}
    }
    else {
	vwgt_sum = nvtxs;
	for (i = 1; i <= nvtxs; i++) {
	    xcm += x[i];
	    ycm += y[i];
	    zcm += z[i];
	}
    }

    xcm /= vwgt_sum;
    ycm /= vwgt_sum;
    zcm /= vwgt_sum;

    /* Generate 6 elements of Inertial tensor. */
    xx = yy = zz = xy = xz = yz = 0.0;
    if (using_vwgts) {
	for (i = 1; i <= nvtxs; i++) {
	    xdif = x[i] - xcm;
	    ydif = y[i] - ycm;
	    zdif = z[i] - zcm;
	    xx += graph[i]->vwgt * xdif * xdif;
	    yy += graph[i]->vwgt * ydif * ydif;
	    zz += graph[i]->vwgt * zdif * zdif;
	    xy += graph[i]->vwgt * xdif * ydif;
	    xz += graph[i]->vwgt * xdif * zdif;
	    yz += graph[i]->vwgt * ydif * zdif;
	}
    }
    else {
	for (i = 1; i <= nvtxs; i++) {
	    xdif = x[i] - xcm;
	    ydif = y[i] - ycm;
	    zdif = z[i] - zcm;
	    xx += xdif * xdif;
	    yy += ydif * ydif;
	    zz += zdif * zdif;
	    xy += xdif * ydif;
	    xz += xdif * zdif;
	    yz += ydif * zdif;
	}
    }

    /* Compute eigenvector with maximum eigenvalue. */

    tensor[0][0] = xx;
    tensor[1][1] = yy;
    tensor[2][2] = zz;
    tensor[0][1] = tensor[1][0] = xy;
    tensor[0][2] = tensor[2][0] = xz;
    tensor[1][2] = tensor[2][1] = yz;
    evals3(tensor, &res, &res, &eval);
    eigenvec3(tensor, eval, evec, &res);

    inertial_axis_time += seconds() - time;

    if (DEBUG_INERTIAL > 0) {
	printf("Principle Axis = (%g, %g, %g)\n  Eval=%g, Residual=%e\n",
	       evec[0], evec[1], evec[2], eval, res);
    }

    /* Allocate space for value array. */

    value = (double *) smalloc((unsigned) (nvtxs + 1) * sizeof(double));

    /* Calculate value to sort/split on for each cell. */
    /* This is inner product with eigenvector. */
    for (i = 1; i <= nvtxs; i++) {
	value[i] = (x[i] - xcm) * evec[0] + (y[i] - ycm) * evec[1] + (z[i] - zcm) * evec[2];
    }

    /* Now find the median value and partition based upon it. */
    space = (int *) smalloc((unsigned) nvtxs * sizeof(int));
    time = seconds();
    rec_median_1(graph, value, nvtxs, space, cube_or_mesh, nsets, goal,
		 using_vwgts, sets, TRUE);
    median_time += seconds() - time;

    sfree((char *) space);
    sfree((char *) value);
}
