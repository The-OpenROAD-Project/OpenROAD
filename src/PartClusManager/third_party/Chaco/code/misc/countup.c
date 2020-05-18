/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include "defs.h"
#include "structs.h"

/* Print metrics of partition quality. */

void      countup(graph, nvtxs, assignment, ndims, architecture, ndims_tot,
		  mesh_dims, print_lev, outfile, using_ewgts)
struct vtx_data **graph;	/* graph data structure */
int       nvtxs;		/* number of vtxs in graph */
short    *assignment;		/* set number of each vtx (length nvtxs+1) */
int       ndims;		/* number of cuts at each level */
int       architecture;		/* what's the target parallel machine? */
int       ndims_tot;		/* total number of hypercube dimensions */
int       mesh_dims[3];		/* extent of mesh in each dimension */
int       print_lev;		/* level of output */
FILE     *outfile;		/* output file if not NULL */
int       using_ewgts;		/* are edge weights being used? */
{
    extern int VERTEX_SEPARATOR;/* vertex instead of edge separator? */
    extern int VERTEX_COVER;	/* make/improve vtx separator via matching? */
    void countup_cube(), countup_mesh(), countup_vtx_sep();

    if (VERTEX_SEPARATOR || VERTEX_COVER) {
	countup_vtx_sep(graph, nvtxs, assignment);
    }
    else {
	if (architecture == 0) {
            countup_cube(graph, nvtxs, assignment, ndims, ndims_tot, print_lev, outfile,
		                 using_ewgts);
	}

	else if (architecture > 0) {
            countup_mesh(graph, nvtxs, assignment, mesh_dims, print_lev, outfile,
		                 using_ewgts);
	}
    }
}
