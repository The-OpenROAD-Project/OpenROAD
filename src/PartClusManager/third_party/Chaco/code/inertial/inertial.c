/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <math.h>
#include <stdio.h>
#include "structs.h"
#include "defs.h"


void      inertial(graph, nvtxs, cube_or_mesh, nsets, igeom, coords, sets,
		             goal, using_vwgts)
struct vtx_data **graph;	/* graph data structure */
int       nvtxs;		/* number of vtxs in graph */
int       cube_or_mesh;		/* 0 => hypercube, d => d-dimensional mesh */
int       nsets;		/* number of sets to cut into */
int       igeom;		/* 1, 2 or 3 dimensional geometry? */
float   **coords;		/* x, y and z coordinates of vertices */
short    *sets;			/* set each vertex gets assigned to */
double   *goal;			/* desired set sizes */
int       using_vwgts;		/* are vertex weights being used? */
{
    extern    int DEBUG_TRACE;	/* trace the execution of the code */
    extern double inertial_time;/* time spend in inertial calculations */
    double    time;		/* timing parameter */
    double    seconds();
    void      inertial1d(), inertial2d(), inertial3d();

    time = seconds();

    if (DEBUG_TRACE > 0) {
	printf("<Entering inertial, nvtxs = %d>\n", nvtxs);
    }

    if (igeom == 1)
	inertial1d(graph, nvtxs, cube_or_mesh, nsets,
		   coords[0], sets, goal, using_vwgts);

    else if (igeom == 2)
	inertial2d(graph, nvtxs, cube_or_mesh, nsets,
		   coords[0], coords[1], sets, goal, using_vwgts);

    else if (igeom == 3)
	inertial3d(graph, nvtxs, cube_or_mesh, nsets,
	       coords[0], coords[1], coords[2], sets, goal, using_vwgts);
    inertial_time += seconds() - time;
}
