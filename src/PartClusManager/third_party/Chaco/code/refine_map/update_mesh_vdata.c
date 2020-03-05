/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"defs.h"
#include	"refine_map.h"


void      update_mesh_vdata(old_loc, new_loc, dim, ewgt, vdata, mesh_dims,
    neighbor, vtx2node)
int       old_loc;		/* previous node for moved vertex in moved dimension */
int       new_loc;		/* new node for moved vertex in moved dimension */
int       dim;			/* dimension that was changed */
float     ewgt;			/* weight of edge */
struct refine_vdata *vdata;	/* array of vertex data */
int       mesh_dims[3];		/* size of processor mesh */
int       neighbor;		/* vertex impacted by flip */
short    *vtx2node;		/* mapping from comm_graph vtxs to processors */
{
    struct refine_vdata *vptr;	/* correct element in vdata */
    int       offset;		/* index into vdata array */
    int       my_loc;		/* my location in relevant dimension */
    int       neighbor_node;	/* processor neighbor assigned to */

    neighbor_node = vtx2node[neighbor];

    if (dim == 0) {
        offset = 0;
	my_loc = neighbor_node % mesh_dims[0];
    }
    else if (dim == 1) {
        offset = mesh_dims[0] * mesh_dims[1] * mesh_dims[2];
	my_loc = (neighbor_node / mesh_dims[0]) % mesh_dims[1];
    }
    else if (dim == 2) {
        offset = 2 * mesh_dims[0] * mesh_dims[1] * mesh_dims[2];
	my_loc = neighbor_node / (mesh_dims[0] * mesh_dims[1]);
    }

    vptr = &vdata[offset + neighbor];

    /* If I'm far away from the flipped edge, I'm not effected. */
    if (!((old_loc < my_loc && new_loc < my_loc) ||
	  (old_loc > my_loc && new_loc > my_loc))) {
	if (old_loc < my_loc) {		/* Old moves to the right to line up with me. */
	    vptr->same += ewgt;
	    vptr->below -= ewgt;
	}

	else if (old_loc > my_loc) {	/* Old moves to the left to line up with me. */
	    vptr->same += ewgt;
	    vptr->above -= ewgt;
	}

	else if (new_loc < my_loc) {	/* Old moves to the left to pass me. */
	    vptr->same -= ewgt;
	    vptr->below += ewgt;
	}

	else if (new_loc > my_loc) {	/* Old moves to the right to pass me. */
	    vptr->same -= ewgt;
	    vptr->above += ewgt;
	}
    }
}
