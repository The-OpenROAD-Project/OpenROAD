/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"structs.h"
#include	"defs.h"
#include	"refine_map.h"


void      compute_cube_vdata(vdata, comm_graph, vtx, mask, vtx2node)
struct refine_vdata *vdata;	/* preference data for a vertex */
struct vtx_data **comm_graph;	/* communication graph data structure */
int       vtx;			/* current vertex */
int       mask;			/* bit set in current hypercube dimension */
short    *vtx2node;		/* maps graph vtxs to mesh nodes */
{
    float     same;		/* my preference to stay where I am */
    float     change;		/* my preference to change this bit */
    float     ewgt;		/* weight of an edge */
    int       neighbor;		/* neighboring vtx in comm_graph */
    int       my_side;		/* which side of hypercube I'm on */
    int       neighb_side;	/* which side of hypercube neighbor's on */
    int       j;		/* loop counter */

    my_side = (vtx2node[vtx] & mask);

    change = 0;
    same = 0;
    for (j = 1; j < comm_graph[vtx]->nedges; j++) {
	neighbor = comm_graph[vtx]->edges[j];
	ewgt = comm_graph[vtx]->ewgts[j];

	neighb_side = (vtx2node[neighbor] & mask);

	if (neighb_side != my_side) {
	    change += ewgt;
	}
	else {
	    same += ewgt;
	}
    }
    vdata->same = same;
    vdata->above = change;
}
