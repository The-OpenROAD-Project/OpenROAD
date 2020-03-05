/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

/* Use same ratios as original goals, but adjust based on set sizes. */
/* Note: mesh stuff only works for division into two sets. */

void      make_subgoal(goal, subgoal, nsets, cube_or_mesh, nsets_tot,
		                 mesh_dims, set, sub_vwgt_sum)
double   *goal;			/* goals for sets */
double   *subgoal;		/* goals for subset of sets */
int       nsets;		/* number of subsets in one partition */
int       cube_or_mesh;		/* 0=> hypercube, d=> d-dimensional mesh */
int       nsets_tot;		/* total number of sets to divide into */
int       mesh_dims[3];		/* shape of mesh */
int       set;			/* which set am I in? */
double    sub_vwgt_sum;		/* sum of subgraph vertex weights */
{
    double    tweight;		/* total weight among all subgoals */
    double    ratio;		/* scaling factor */
    int       index;		/* x, y or z location of a processor */
    int       sub_nsets;	/* largest number of processors in submesh */
    int       xstart, xwidth;	/* parameters describing submesh */
    int       i, j, x;		/* loop counters */

    if (!cube_or_mesh) {	/* First do hypercube case. */
	tweight = 0;
	for (j = 0, i = set; i < nsets_tot; i += nsets, j++) {
	    subgoal[j] = goal[i];
	    tweight += goal[i];
	}
	sub_nsets = nsets_tot / nsets;
    }

    else {
	if (set == 0) {
	    xstart = 0;
	    xwidth = mesh_dims[0] - mesh_dims[0] / 2;
	}
	else {
	    xwidth = mesh_dims[0] / 2;
	    xstart = mesh_dims[0] - mesh_dims[0] / 2;
	}
	i = 0;
	tweight = 0;
	index = xstart;
	for (x = xstart; x < xstart + xwidth; x++) {
	    subgoal[i] = goal[index++];
	    tweight += subgoal[i++];
	}
	sub_nsets = xwidth;
    }

    ratio = sub_vwgt_sum / tweight;
    for (i = 0; i < sub_nsets; i++) {
	subgoal[i] *= ratio;
    }
}
