/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include "defs.h"
#include "structs.h"

/* Make coarse graph vertex coordinates be center-of-mass of their */
/* fine graph constituents. */

void      makeccoords(graph, cnvtxs, cv2v_ptrs, cv2v_vals,
		                igeom, coords, ccoords)
struct vtx_data **graph;	/* array of vtx data for graph */
int       cnvtxs;		/* number of vertices in coarse graph */
int       igeom;		/* dimensions of geometric data */
int      *cv2v_ptrs;		/* vtxs corresponding to each cvtx */
int      *cv2v_vals;		/* indices into cv2v_vals */
float   **coords;		/* coordinates for vertices */
float   **ccoords;		/* coordinates for coarsened vertices */
{
    double    mass;		/* total mass of merged vertices */
    float    *cptr;		/* loops through ccoords */
    int       cvtx;		/* coarse graph vertex */
    int       vtx;		/* vertex being merged */
    int       i, j;		/* loop counters */
    double   *smalloc();

    for (i = 0; i < igeom; i++) {
	ccoords[i] = cptr = (float *) smalloc((unsigned) (cnvtxs + 1) * sizeof(float));
	for (cvtx = cnvtxs; cvtx; cvtx--) {
	    *(++cptr) = 0;
	}
    }
    if (igeom == 1) {
	for (cvtx = 1; cvtx <= cnvtxs; cvtx++) {
	    mass = 0;
	    for (j = cv2v_ptrs[cvtx]; j < cv2v_ptrs[cvtx + 1]; j++) {
		vtx = cv2v_vals[j];
		mass += graph[vtx]->vwgt;
		ccoords[0][cvtx] += graph[vtx]->vwgt * coords[0][vtx];
	    }
	    ccoords[0][cvtx] /= mass;
	}
    }
    else if (igeom == 2) {
	for (cvtx = 1; cvtx <= cnvtxs; cvtx++) {
	    mass = 0;
	    for (j = cv2v_ptrs[cvtx]; j < cv2v_ptrs[cvtx + 1]; j++) {
		vtx = cv2v_vals[j];
		mass += graph[vtx]->vwgt;
		ccoords[0][cvtx] += graph[vtx]->vwgt * coords[0][vtx];
		ccoords[1][cvtx] += graph[vtx]->vwgt * coords[1][vtx];
	    }
	    ccoords[0][cvtx] /= mass;
	    ccoords[1][cvtx] /= mass;
	}
    }
    else if (igeom > 2) {
	for (cvtx = 1; cvtx <= cnvtxs; cvtx++) {
	    mass = 0;
	    for (j = cv2v_ptrs[cvtx]; j < cv2v_ptrs[cvtx + 1]; j++) {
		vtx = cv2v_vals[j];
		mass += graph[vtx]->vwgt;
		ccoords[0][cvtx] += graph[vtx]->vwgt * coords[0][vtx];
		ccoords[1][cvtx] += graph[vtx]->vwgt * coords[1][vtx];
		ccoords[2][cvtx] += graph[vtx]->vwgt * coords[2][vtx];
	    }
	    ccoords[0][cvtx] /= mass;
	    ccoords[1][cvtx] /= mass;
	    ccoords[2][cvtx] /= mass;
	}
    }
}
