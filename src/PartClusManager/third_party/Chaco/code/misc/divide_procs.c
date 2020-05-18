/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include "params.h"
#include "defs.h"
#include "structs.h"


int       divide_procs(architecture, ndims, ndims_tot, set_info, set, subsets, inert,
            pndims_real, pnsets_real, pstriping, cut_dirs, mesh_dims, hops_special)
int       architecture;		/* 0 => hypercube, d => d-dimensional mesh */
int       ndims;		/* normal dimension of each cut */
int       ndims_tot;		/* total number of hypercube dimensions */
struct set_info *set;		/* data for set being divided */
struct set_info *set_info;	/* data for all sets */
short    *subsets;		/* subsets to be created */
int       inert;		/* using inertial method? */
int      *pndims_real;		/* actual ndims for this cut */
int      *pnsets_real;		/* # sets created by this cut */
int      *pstriping;		/* cut in single direction? */
int      *cut_dirs;		/* direction of each cut if mesh */
int      *mesh_dims;		/* size of full mesh */
short     hops_special[][MAXSETS];	/* hop matrix for nonstandard cases */
{
    int       nsets_real;	/* number of sets to divide into */
    int       ndims_real;	/* number of eigenvectors to use */
    int       striping;		/* cut in single direction? */
    int       flag;		/* unusual partition => use special hops */
    int       ndim_poss;	/* largest dimensionality possible */
    int       idims;		/* true dimensionality of subgrid */
    int       i;		/* loop counter */
    int       define_submeshes(), define_subcubes();

    if (architecture > 0) {	/* Mesh, complicated case. */
	nsets_real = set->span[0] * set->span[1] * set->span[2];
	nsets_real = min(1 << ndims, nsets_real);
	ndims_real = ndims;
	while (1 << ndims_real > nsets_real)
	    --ndims_real;

        ndim_poss = 0;
	idims = 0;
	for (i = 0; i < 3; i++) {
	    if (set->span[i] >= 2) {
		ndim_poss++;
		idims++;
	    }
	    if (set->span[i] >= 4) ndim_poss++;
	    if (set->span[i] >= 8) ndim_poss++;
	}
	ndims_real = min(ndim_poss, ndims_real);

	if (idims > 1) {
	    nsets_real = 1 << ndims_real;
	}


	flag = define_submeshes(nsets_real, architecture, mesh_dims, set,
		      set_info, subsets, inert, &striping, cut_dirs, hops_special);
	if (striping) {
	    ndims_real = 1;
	}
    }

    else if (architecture == 0) {	/* Hypercube, easy case. */
	ndims_real = min(ndims, set->ndims);
	nsets_real = 1 << ndims_real;

	flag = define_subcubes(nsets_real, ndims_tot, ndims_real, set,
		      set_info, subsets, inert, &striping, hops_special);

	if (striping)
	    ndims_real = 1;
    }

    *pndims_real = ndims_real;
    *pnsets_real = nsets_real;
    *pstriping = striping;

    return (flag);
}
