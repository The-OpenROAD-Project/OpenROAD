/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"structs.h"
#include	"defs.h"
#include	"refine_map.h"


/* Use a greedy strategy to swap assignments to reduce hops. */
/* Note that because of our graph data structure, set assignments in the graph */
/* begin at 1 instead of at 0. */
int       refine_mesh(comm_graph, cube_or_mesh, mesh_dims, maxdesire,
    vtx2node, node2vtx)
struct vtx_data **comm_graph;	/* graph for communication requirements */
int       cube_or_mesh;		/* number of dimensions in mesh */
int       mesh_dims[3];		/* dimensions of mesh */
double    maxdesire;		/* largest possible desire to flip an edge */
short    *vtx2node;		/* mapping from comm_graph vtxs to mesh nodes */
short    *node2vtx;		/* mapping from mesh nodes to comm_graph vtxs */
{
    struct refine_vdata *vdata = NULL;	/* desire data for all vertices */
    struct refine_vdata *vptr;	/* loops through vdata */
    struct refine_edata *edata = NULL;	/* desire data for all edges */
    struct refine_edata *eguy;	/* one element in edata array */
    struct refine_edata **desire_ptr = NULL;	/* array of desire buckets */
    double   *desires = NULL;	/* each edge's inclination to flip */
    int      *indices = NULL;	/* sorted list of desire values */
    int      *space = NULL;	/* used for sorting disire values */
    double    best_desire;	/* highest desire of edge to flip */
    int       imax;		/* maxdesire rounded up */
    int       nsets_tot;	/* total number of sets/processors */
    int       neighbor;		/* neighboring vertex */
    int       dim;		/* loops over mesh dimensions */
    int       nwires;		/* number of wires in processor mesh */
    int       wire;		/* loops through all wires */
    int       node1, node2;	/* processors joined by a wire */
    int       vtx1, vtx2;	/* corresponding vertices in comm_graph */
    int       loc1, loc2;	/* location of vtxs in flipping dimension */
    int       error;		/* out of space? */
    int       i, j, k;		/* loop counter */
    double   *smalloc_ret(), find_maxdeg();
    double    compute_mesh_edata();
    int       sfree();
    void       compute_mesh_vdata(), init_mesh_edata(), mergesort();
    void      update_mesh_vdata(), update_mesh_edata();

    error = 1;

    nsets_tot = mesh_dims[0] * mesh_dims[1] * mesh_dims[2];

    imax = maxdesire;
    if (imax != maxdesire) imax++;

    vdata = (struct refine_vdata *)
       smalloc_ret((unsigned) (cube_or_mesh * nsets_tot + 1) *
			  sizeof(struct refine_vdata));
    if (vdata == NULL) goto skip;

    /* Compute each node's desires to move or stay put. */
    vptr = vdata;
    for (dim=0; dim<cube_or_mesh; dim++) {
        for (i = 1; i <= nsets_tot; i++) {
	    compute_mesh_vdata(++vptr, comm_graph, i, vtx2node, mesh_dims, dim);
	}
    }

    nwires = (mesh_dims[0] - 1) * mesh_dims[1] * mesh_dims[2] +
       mesh_dims[0] * (mesh_dims[1] - 1) * mesh_dims[2] +
       mesh_dims[0] * mesh_dims[1] * (mesh_dims[2] - 1);

    edata = (struct refine_edata *) smalloc_ret((unsigned) (nwires + 1) * sizeof(struct refine_edata));
    desires = (double *) smalloc_ret((unsigned) nwires * sizeof(double));
    if (vdata == NULL || desires == NULL) goto skip;

    /* Initialize all the edge values. */
    init_mesh_edata(edata, mesh_dims);
    for (wire = 0; wire < nwires; wire++) {
	desires[wire] = edata[wire].swap_desire =
	    compute_mesh_edata(&(edata[wire]), vdata, mesh_dims, comm_graph,
		node2vtx);
    }

    /* Set special value for end pointer. */
    edata[nwires].swap_desire = 2 * find_maxdeg(comm_graph, nsets_tot, TRUE, (float *) NULL);

    /* I now need to sort all the wire preference values */
    indices = (int *) smalloc_ret((unsigned) nwires * sizeof(int));
    space = (int *) smalloc_ret((unsigned) nwires * sizeof(int));
    if (indices == NULL || space == NULL) goto skip;

    mergesort(desires, nwires, indices, space);

    sfree((char *) space);
    sfree((char *) desires);
    space = NULL;
    desires = NULL;

    best_desire = (edata[indices[nwires-1]]).swap_desire;

    /* Now construct a buckets of linked lists with desire values. */

    if (best_desire > 0) {
        desire_ptr = (struct refine_edata **)
	    smalloc_ret((unsigned) (2*imax + 1) * sizeof(struct refine_edata *));
        if (desire_ptr == NULL) goto skip;

        for (i=2*imax; i>= 0; i--) desire_ptr[i] = NULL;

        for (i = nwires - 1; i >= 0; i--) {
	    eguy = &(edata[indices[i]]);
	    /* Round the swap desire up. */
	    if (eguy->swap_desire >= 0) {
	        k = eguy->swap_desire;
	        if (k != eguy->swap_desire) k++;
	    }
	    else {
	        k = -eguy->swap_desire;
	        if (k != -eguy->swap_desire) k++;
	        k = -k;
	    }

	    k += imax;

	    eguy->prev = NULL;
	    eguy->next = desire_ptr[k];
	    if (desire_ptr[k] != NULL) desire_ptr[k]->prev = eguy;
	    desire_ptr[k] = eguy;
        }
    }
    else {
	desire_ptr = NULL;
    }

    sfree((char *) indices);
    indices = NULL;


    /* Everything is now set up.  Swap sets across wires until no more improvement. */

    while (best_desire > 0) {
	k = best_desire + 1 + imax;
	if (k > 2*imax) k = 2*imax;
	while (k > imax && desire_ptr[k] == NULL) k--;
	eguy = desire_ptr[k];

	dim = eguy->dim;
	node1 = eguy->node1;
	node2 = eguy->node2;
	vtx1 = node2vtx[node1];
	vtx2 = node2vtx[node2];
	if (dim == 0) {
	    loc1 = node1 % mesh_dims[0];
	    loc2 = node2 % mesh_dims[0];
	}
	else if (dim == 1) {
	    loc1 = (node1 / mesh_dims[0]) % mesh_dims[1];
	    loc2 = (node2 / mesh_dims[0]) % mesh_dims[1];
	}
	else if (dim == 2) {
	    loc1 = node1 / (mesh_dims[0] * mesh_dims[1]);
	    loc2 = node2 / (mesh_dims[0] * mesh_dims[1]);
	}

	/* Now swap the vertices. */
	node2vtx[node1] = (short) vtx2;
	node2vtx[node2] = (short) vtx1;
	vtx2node[vtx1] = (short) node2;
	vtx2node[vtx2] = (short) node1;

	/* First update all the vdata fields for vertices effected by this flip. */
	for (j = 1; j < comm_graph[vtx1]->nedges; j++) {
	    neighbor = comm_graph[vtx1]->edges[j];
	    if (neighbor != vtx2)
		update_mesh_vdata(loc1, loc2, dim, comm_graph[vtx1]->ewgts[j],
		    vdata, mesh_dims, neighbor, vtx2node);
	}

	for (j = 1; j < comm_graph[vtx2]->nedges; j++) {
	    neighbor = comm_graph[vtx2]->edges[j];
	    if (neighbor != vtx1)
		update_mesh_vdata(loc2, loc1, dim, comm_graph[vtx2]->ewgts[j],
		    vdata, mesh_dims, neighbor, vtx2node);
	}

	/* Now recompute all preferences for vertices that were moved. */
	for (j=0; j<cube_or_mesh; j++) {
	    compute_mesh_vdata(&(vdata[j*nsets_tot + vtx1]), comm_graph,
		vtx1, vtx2node, mesh_dims, j);
	    compute_mesh_vdata(&(vdata[j*nsets_tot + vtx2]), comm_graph,
		vtx2, vtx2node, mesh_dims, j);
	}

	/* Now I can update the values of all the edges associated with all the
	   effected vertices.  Note that these include mesh neighbors of node1 and
	   node2 in addition to the dim-edges of graph neighbors of vtx1 and vtx2. */

	/* For each neighbor vtx, look at -1 and +1 edge.  If desire hasn't changed,
	   return.  Otherwise, pick him up and move him. Similarly for all
	   directional neighbors of node1 and node2. */

	for (j = 1; j < comm_graph[vtx1]->nedges; j++) {
	    neighbor = comm_graph[vtx1]->edges[j];
	    if (neighbor != vtx2)
		update_mesh_edata(neighbor, dim, edata, vdata, comm_graph,
		    mesh_dims, node2vtx, vtx2node, &best_desire, imax, desire_ptr);
	}

	for (j = 1; j < comm_graph[vtx2]->nedges; j++) {
	    neighbor = comm_graph[vtx2]->edges[j];
	    if (neighbor != vtx1)
		update_mesh_edata(neighbor, dim, edata, vdata, comm_graph,
		    mesh_dims, node2vtx, vtx2node, &best_desire, imax, desire_ptr);
	}
	for (j = 0; j < cube_or_mesh; j++) {
	    update_mesh_edata(vtx1, j, edata, vdata, comm_graph, mesh_dims,
		node2vtx, vtx2node, &best_desire, imax, desire_ptr);
	    update_mesh_edata(vtx2, j, edata, vdata, comm_graph, mesh_dims,
		node2vtx, vtx2node, &best_desire, imax, desire_ptr);
	}

	k = best_desire + 1 + imax;
	if (k > 2*imax) k = 2*imax;
	while (k > imax && desire_ptr[k] == NULL) k--;
	best_desire = k - imax;
    }
    error = 0;

skip:
    sfree((char *) indices);
    sfree((char *) space);
    sfree((char *) desires);
    sfree((char *) desire_ptr);
    sfree((char *) vdata);
    sfree((char *) edata);

    return(error);
}
