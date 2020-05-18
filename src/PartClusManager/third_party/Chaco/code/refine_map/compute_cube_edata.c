/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"structs.h"
#include	"defs.h"
#include	"refine_map.h"


double    compute_cube_edata(edata, vdata, nsets_tot, comm_graph, node2vtx)
struct refine_edata *edata;	/* desire data for current edge */
struct refine_vdata *vdata;	/* data for all vertices */
int       nsets_tot;		/* total number of processors */
struct vtx_data **comm_graph;	/* communication graph */
short    *node2vtx;		/* maps mesh nodes to graph vertices */
{
    double    desire;		/* edge's interest in flipping */
    float     ewgt;		/* edge weight */
    int       offset;		/* offset into vdata array */
    int       vtx1, vtx2;	/* vertices on either side of wire */
    int       is_an_edge();

    vtx1 = node2vtx[edata->node1];
    vtx2 = node2vtx[edata->node2];
    offset = nsets_tot * edata->dim;

    desire = (vdata[offset + vtx1].above - vdata[offset + vtx1].same) +
       (vdata[offset + vtx2].above - vdata[offset + vtx2].same);

    /* Subtract off potential doubly counted edge. */
    if (is_an_edge(comm_graph[vtx1], vtx2, &ewgt)) {
	desire -= 2 * ewgt;
    }

    return (desire);
}
