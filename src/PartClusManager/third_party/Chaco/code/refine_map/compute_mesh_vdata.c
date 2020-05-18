/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"structs.h"
#include	"defs.h"
#include	"refine_map.h"


void      compute_mesh_vdata(vdata, comm_graph, vtx, vtx2node, mesh_dims, dim)
struct refine_vdata *vdata;	/* preference data for a vertex */
struct vtx_data **comm_graph;	/* communication graph data structure */
int       vtx;			/* current vertex */
short    *vtx2node;		/* maps graph vtxs to mesh nodes */
int       mesh_dims[3];		/* size of mesh */
int       dim;			/* dimension we are currently working in */
{
    float     above;		/* my preference to move up in each dimension */
    float     below;		/* my preference to move down in each dimension */
    float     same;		/* my preference to stay where I am */
    short     my_loc;		/* my location in mesh */
    int       neighb_loc;	/* neighbor's location in mesh */
    float     ewgt;		/* weight of an edge */
    int       node;		/* set vertex is assigned to */
    int       neighbor;		/* neighboring vtx in comm_graph */
    int       j;		/* loop counter */

    node = vtx2node[vtx];

    if (dim == 0) {
	my_loc = node % mesh_dims[0];
    }
    else if (dim == 1) {
	my_loc = (node / mesh_dims[0]) % mesh_dims[1];
    }
    else if (dim == 2) {
	my_loc = node / (mesh_dims[0] * mesh_dims[1]);
    }

    below = above = same = 0;
    for (j = 1; j < comm_graph[vtx]->nedges; j++) {
	neighbor = comm_graph[vtx]->edges[j];
	ewgt = comm_graph[vtx]->ewgts[j];
	node = vtx2node[neighbor];

	if (dim == 0) {
	    neighb_loc = node % mesh_dims[0];
	}
	else if (dim == 1) {
	    neighb_loc = (node / mesh_dims[0]) % mesh_dims[1];
	}
	else if (dim == 2) {
	    neighb_loc = node / (mesh_dims[0] * mesh_dims[1]);
	}

	if (neighb_loc < my_loc)
	    below += ewgt;
	else if (neighb_loc > my_loc)
	    above += ewgt;
	else
	    same += ewgt;
    }
    vdata->below = below;
    vdata->above = above;
    vdata->same = same;
}
