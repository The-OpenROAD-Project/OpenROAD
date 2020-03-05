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
int       refine_cube(comm_graph, ndims_tot, maxdesire, vtx2node, node2vtx)
struct vtx_data **comm_graph;	/* graph for communication requirements */
int       ndims_tot;		/* dimensionality of hypercube */
double    maxdesire;		/* largest possible desire to flip an edge */
short    *vtx2node;		/* mapping from comm_graph vtxs to processors */
short    *node2vtx;		/* mapping from processors to comm_graph vtxs */
{
    struct refine_vdata *vdata = NULL;	/* desire data for vertices */
    struct refine_vdata *vptr;	/* loops through vdata */
    struct refine_edata *edata = NULL;	/* desire data for edges */
    struct refine_edata *eptr;	/* loops through edata */
    struct refine_edata *eguy;	/* one element in edata array */
    struct refine_edata **desire_ptr = NULL;	/* array of desire buckets */
    double   *desires = NULL;	/* each edge's inclination to flip */
    double   *dptr;		/* loops through desire */
    int      *indices = NULL;	/* sorted list of desire values */
    int      *space = NULL;	/* used for sorting disire values */
    double    best_desire;	/* desire of max edge to flip */
    int       imax;		/* maxdesire rounded up */
    int       nsets_tot;	/* total number of sets/processors */
    int       neighbor;		/* neighboring vertex */
    int       dim;		/* loops over cube dimensions */
    int       mask;		/* bit set for current dimension */
    int       side;		/* side of hypercube node is on */
    int       nwires;		/* number of wires in dimension of hypercube */
    int       nwires_tot;	/* total number of wires in hypercube */
    int       wire;		/* loops through all wires */
    int       node1, node2;	/* processors joined by a wire */
    int       vtx1, vtx2;	/* corresponding vertices in comm_graph */
    int       error;		/* out of space? */
    int       i, j, k;		/* loop counter */
    double   *smalloc_ret(), find_maxdeg();
    double    compute_cube_edata();
    int       sfree();
    void      compute_cube_vdata(), init_cube_edata(), mergesort();
    void      update_cube_vdata(), update_cube_edata();

    nsets_tot = 1 << ndims_tot;
    error = 1;

    imax = maxdesire;
    if (imax != maxdesire) imax++;

    /* This is really just ndims_tot different 1-D problems. */

    /* Allocate space for and inititalize the vertex data. */
    vdata = (struct refine_vdata *)
	    smalloc_ret((unsigned) (ndims_tot*nsets_tot + 1) * sizeof(struct refine_vdata));
    if (vdata == NULL) goto skip;

    /* Compute each node's desires to move or stay put in each direction. */
    vptr = vdata;
    for (dim=0; dim<ndims_tot; dim++) {
	mask = 1<<dim;
        for (i = 1; i <= nsets_tot; i++) {
	    compute_cube_vdata(++vptr, comm_graph, i, mask, vtx2node);
	}
    }

    /* Now allocate space for and initialize the wire data. */
    nwires = nsets_tot/2;
    nwires_tot = nwires * ndims_tot;

    edata = (struct refine_edata *)
	    smalloc_ret((unsigned) (nwires_tot + 1) * sizeof(struct refine_edata));
    if (edata == NULL) goto skip;

    desires = (double *) smalloc_ret((unsigned) nwires_tot * sizeof(double));
    if (desires == NULL) goto skip;

    /* Initialize all the wire swap_desire values. */
    eptr = edata;
    dptr = desires;
    i = 0;
    for (dim=0; dim<ndims_tot; dim++) {
	mask = 1<<dim;
	for (wire = 0; 2*wire < nsets_tot; wire++) {
	    /* Insert zero bit at position dim. */
	    j = (wire >> dim) << (dim+1);
	    i = (wire << 1) ^ j;
	    j ^= (i>>1);
	    init_cube_edata(eptr, j, dim, mask);
	    *dptr++ = eptr->swap_desire = 
		compute_cube_edata(eptr, vdata, nsets_tot, comm_graph, node2vtx);
	    eptr++;
	}
    }

    /* Set value for end pointer larger than all others. */
    edata[nwires_tot].swap_desire =
       2 * find_maxdeg(comm_graph, nsets_tot, TRUE, (float *) NULL);

    /* I now need to sort all the wire preference values */
    indices = (int *) smalloc_ret((unsigned) nwires_tot * sizeof(int));
    space = (int *) smalloc_ret((unsigned) nwires_tot * sizeof(int));
    if (indices == NULL || space == NULL) goto skip;

    mergesort(desires, nwires_tot, indices, space);

    sfree((char *) space);
    sfree((char *) desires);
    space = NULL;
    desires = NULL;

    best_desire = (edata[indices[nwires_tot-1]]).swap_desire;

    /* Now construct buckets of linked lists with desire values. */

    if (best_desire > 0) {
        desire_ptr = (struct refine_edata **)
	    smalloc_ret((unsigned) (2*imax + 1) * sizeof(struct refine_edata *));
        if (desire_ptr == NULL) goto skip;
        for (i=2*imax; i>= 0; i--) desire_ptr[i] = NULL;

        for (i = nwires_tot - 1; i >= 0; i--) {
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
	mask = 1<<dim;
	node1 = eguy->node1;
	node2 = eguy->node2;
	vtx1 = node2vtx[node1];
	vtx2 = node2vtx[node2];

	/* Swap the sets. */
	node2vtx[node1] = (short) vtx2;
	node2vtx[node2] = (short) vtx1;
	vtx2node[vtx1] = (short) node2;
	vtx2node[vtx2] = (short) node1;

	/* Update all the vdata fields for vertices effected by this flip. */
	/* First do the vertices adjacent to swapped guys, in swapped dimension. */
	side = node1 & mask;
	for (j = 1; j < comm_graph[vtx1]->nedges; j++) {
	    neighbor = comm_graph[vtx1]->edges[j];
	    if (neighbor != vtx2)
		update_cube_vdata(side, mask, vtx2node[neighbor],
		    comm_graph[vtx1]->ewgts[j], &(vdata[dim*nsets_tot + neighbor]));
	}

	side = node2 & mask;
	for (j = 1; j < comm_graph[vtx2]->nedges; j++) {
	    neighbor = comm_graph[vtx2]->edges[j];
	    if (neighbor != vtx1)
		update_cube_vdata(side, mask, vtx2node[neighbor], 
		    comm_graph[vtx2]->ewgts[j], &(vdata[dim*nsets_tot + neighbor]));
	}

	/* Now recompute all preferences for vertices that were moved. */
	for (j=0; j<ndims_tot; j++) {
	    k = 1<<j;
	    compute_cube_vdata(&(vdata[j*nsets_tot + vtx1]), comm_graph, vtx1, k, vtx2node);
	    compute_cube_vdata(&(vdata[j*nsets_tot + vtx2]), comm_graph, vtx2, k, vtx2node);
	}


	/* Now I can update the values of all the edges associated with all the
	   effected vertices.  Note that these include cube neighbors of node1 and
	   node2 in addition to the dim-edges of graph neighbors of vtx1 and vtx2. */

	/* For each neighbor vtx, look at wire in this direction.  If desire hasn't changed,
	   return.  Otherwise, pick him up and move him in desire list. Similarly for all
	   directional neighbors of node1 and node2. */

	for (j = 1; j < comm_graph[vtx1]->nedges; j++) {
	    neighbor = comm_graph[vtx1]->edges[j];
	    if (neighbor != vtx2)
		update_cube_edata(neighbor, dim, edata, vdata, comm_graph, node2vtx,
		    vtx2node, nsets_tot, &best_desire, imax, desire_ptr);
	}

	for (j = 1; j < comm_graph[vtx2]->nedges; j++) {
	    neighbor = comm_graph[vtx2]->edges[j];
	    if (neighbor != vtx1)
		update_cube_edata(neighbor, dim, edata, vdata, comm_graph, node2vtx,
		    vtx2node, nsets_tot, &best_desire, imax, desire_ptr);
	}
	for (j = 0; j < ndims_tot; j++) {
	    update_cube_edata(vtx1, j, edata, vdata, comm_graph,
		node2vtx, vtx2node, nsets_tot, &best_desire, imax, desire_ptr);
	    update_cube_edata(vtx2, j, edata, vdata, comm_graph,
		node2vtx, vtx2node, nsets_tot, &best_desire, imax, desire_ptr);
	}

	k = best_desire + 1 + imax;
	if (k > 2*imax) k = 2*imax;
	while (k > imax && desire_ptr[k] == NULL) k--;
	best_desire = k - imax;
    }
    error = 0;

skip:
    sfree((char *) space);
    sfree((char *) desires);
    sfree((char *) indices);
    sfree((char *) desire_ptr);
    sfree((char *) vdata);
    sfree((char *) edata);

    return(error);
}
