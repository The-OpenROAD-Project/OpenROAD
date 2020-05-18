/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include "defs.h"
#include "structs.h"


/* Find a maximal matching in a graph by geometrically near neighbors. */

int       maxmatch5(graph, nvtxs, mflag, igeom, coords)
struct vtx_data **graph;	/* array of vtx data for graph */
int       nvtxs;		/* number of vertices in graph */
int      *mflag;		/* flag indicating vtx selected or not */
int       igeom;		/* geometric dimensionality */
float   **coords;		/* coordinates of each vertex */
{
    extern double DOUBLE_MAX;	/* largest floating point value */
    double    dist;		/* distance to free neighbor */
    double    min_dist;		/* smallest distance to free neighbor */
    int      *jptr;		/* loops through integer arrays */
    int       vtx;		/* vertex to process next */
    int       neighbor;		/* neighbor of a vertex */
    int       nmerged;		/* number of edges in matching */
    int       jsave;		/* best edge so far */
    int       i, j, k;		/* loop counters */
    double    drandom();

    /* Initialize mflag array. */
    jptr = mflag;
    for (i = 1; i <= nvtxs; i++) {
	*(++jptr) = 0;
    }

    nmerged = 0;

    /* Select random starting point in list of vertices. */
    vtx = 1 + drandom() * nvtxs;

    if (igeom == 1) {
	for (i = nvtxs; i; i--) {	/* Choose geometrically nearest neighbor */
	    if (mflag[vtx] == 0) {	/* Not already matched. */
		/* Select nearest free edge. */
		jsave = 0;
		min_dist = DOUBLE_MAX;
		for (j = 1; j < graph[vtx]->nedges; j++) {
		    neighbor = graph[vtx]->edges[j];
		    if (mflag[neighbor] == 0) {
			dist = (coords[0][vtx] - coords[0][neighbor]) *
			   (coords[0][vtx] - coords[0][neighbor]);
			if (dist < min_dist) {
			    jsave = j;
			    min_dist = dist;
			}
		    }
		}
		if (jsave > 0) {
		    neighbor = graph[vtx]->edges[jsave];
		    mflag[vtx] = neighbor;
		    mflag[neighbor] = vtx;
		    nmerged++;
		}
	    }
	    if (++vtx > nvtxs)
		vtx = 1;
	}
    }

    else if (igeom == 2) {
	for (i = nvtxs; i; i--) {	/* Choose geometrically nearest neighbor */
	    if (mflag[vtx] == 0) {	/* Not already matched. */
		/* Select nearest free edge. */
		jsave = 0;
		min_dist = DOUBLE_MAX;
		for (j = 1; j < graph[vtx]->nedges; j++) {
		    neighbor = graph[vtx]->edges[j];
		    if (mflag[neighbor] == 0) {
			dist = (coords[0][vtx] - coords[0][neighbor]) *
			   (coords[0][vtx] - coords[0][neighbor]);
			if (dist < min_dist) {
			    dist += (coords[1][vtx] - coords[1][neighbor]) *
			       (coords[1][vtx] - coords[1][neighbor]);
			    if (dist < min_dist) {
				jsave = j;
				min_dist = dist;
			    }
			}
		    }
		}
		if (jsave > 0) {
		    neighbor = graph[vtx]->edges[jsave];
		    mflag[vtx] = neighbor;
		    mflag[neighbor] = vtx;
		    nmerged++;
		}
	    }
	    if (++vtx > nvtxs)
		vtx = 1;
	}
    }

    else if (igeom >= 2) {
	for (i = nvtxs; i; i--) {	/* Choose geometrically nearest neighbor */
	    if (mflag[vtx] == 0) {	/* Not already matched. */
		/* Select nearest free edge. */
		jsave = 0;
		min_dist = DOUBLE_MAX;
		for (j = 1; j < graph[vtx]->nedges; j++) {
		    neighbor = graph[vtx]->edges[j];
		    if (mflag[neighbor] == 0) {
			dist = (coords[0][vtx] - coords[0][neighbor]) *
			   (coords[0][vtx] - coords[0][neighbor]);
			if (dist < min_dist) {
			    dist += (coords[1][vtx] - coords[1][neighbor]) *
			       (coords[1][vtx] - coords[1][neighbor]);
			    if (dist < min_dist) {
				dist += (coords[2][vtx] - coords[2][neighbor]) *
				   (coords[2][vtx] - coords[2][neighbor]);
				if (dist < min_dist) {
				    jsave = j;
				    min_dist = dist;
				}
			    }
			}
		    }
		}
		if (jsave > 0) {
		    neighbor = graph[vtx]->edges[jsave];
		    mflag[vtx] = neighbor;
		    mflag[neighbor] = vtx;
		    nmerged++;
		}
	    }
	    if (++vtx > nvtxs)
		vtx = 1;
	}
    }

    return (nmerged);
}
