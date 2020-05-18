/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include "defs.h"
#include "params.h"
#include "structs.h"


/* Figure out how to divide mesh into pieces.  Return true if nonstandard. */
int       define_submeshes(nsets, cube_or_mesh, mesh_dims, set, set_info, subsets,
			             inert, striping, dir, hop_mtx_special)
int       nsets;		/* number of subsets in this partition */
int       cube_or_mesh;		/* 0=> hypercube, d=> d-dimensional mesh */
int      *mesh_dims;		/* shape of mesh */
struct set_info *set;		/* set data for set I'm partitioning */
struct set_info *set_info;	/* set data for all sets */
short    *subsets;		/* subsets being created by partition */
int       inert;		/* using inertial method? */
int      *dir;			/* directions of each cut */
int      *striping;		/* should I partition with parallel cuts? */
short     hop_mtx_special[MAXSETS][MAXSETS];	/* hops values if unusual */
{
    extern int KL_METRIC;	/* 2 => using hops, so generate special values */
    int       ndims;		/* dimension of cut */
    int       dims[3];		/* local copy of mesh_dims to modify */
    int       maxdim;		/* longest dimension of the mesh */
    int       mindim;		/* shortest dimension of mesh */
    int       start[3];		/* start in each index of submesh */
    int       width[3];		/* length in each index of submesh */
    int       nbits[3];		/* values for computing hops */
    int       coords[3];	/* location of set in logical grid */
    int       mask[3];		/* values for computing hops */
    int       setnum;		/* number of created set */
    int       flag;		/* return condition */
    int       snaking;		/* is single stripe snaking through grid? */
    int       reverse;		/* should I reverse direction for embedding? */
    int       i, j, k;		/* loop counters */
    int       abs();

    dims[0] = set->span[0];
    dims[1] = set->span[1];
    dims[2] = set->span[2];

    ndims = 1;
    while ((2 << ndims) <= nsets)
	ndims++;

    /* Find the shortest and longest directions in mesh. */
    maxdim = -1;
    mindim = dims[0];
    dir[1] = dir[2] = 0;
    for (i = 0; i < cube_or_mesh; i++) {
	if (dims[i] > maxdim) {
	    maxdim = dims[i];
	    dir[0] = i;
	}
	if (dims[i] < mindim)
	    mindim = dims[i];
    }

    /* Decide whether or not to force striping. */
    i = 0;
    for (j = 0; j < cube_or_mesh; j++)
	if (set->span[j] > 1)
	    i++;

    *striping = (i <= 1 || nsets == 3 ||
		 (maxdim > nsets &&
		  (maxdim > .6 * nsets * mindim || (inert && nsets > 2))));

    snaking = !*striping && inert && nsets > 2;

    if (!*striping) {
	if (nsets >= 4) {	/* Find direction of second & third cuts. */
	    dims[dir[0]] /= 2;
	    maxdim = -1;
	    for (i = 0; i < cube_or_mesh; i++) {
		if (dims[i] > maxdim) {
		    maxdim = dims[i];
		    dir[1] = i;
		}
	    }
	}

	if (nsets == 8) {	/* Find a third direction. */
	    dims[dir[1]] /= 2;
	    maxdim = -1;
	    for (i = 0; i < cube_or_mesh; i++) {
		if (dims[i] > maxdim) {
		    maxdim = dims[i];
		    dir[2] = i;
		}
	    }
	}
	nbits[0] = nbits[1] = nbits[2] = 0;
	for (i = 0; i < ndims; i++)
	    ++nbits[dir[i]];
	for (i = 0; i < 3; i++) {
	    mask[i] = (1 << nbits[i]) - 1;
	}
	mask[1] <<= nbits[0];
	mask[2] <<= nbits[0] + nbits[1];
    }

    for (k = nsets - 1; k >= 0; k--) {	/* Backwards to not overwrite current set. */

	for (i = 0; i < 3; i++) {
	    start[i] = 0;
	    width[i] = dims[i] = set->span[i];
	}

	if (*striping) {	/* Use longest direction for all cuts. */
	    start[dir[0]] = (k * dims[dir[0]] + nsets - 1) / nsets;
	    width[dir[0]] = ((k + 1) * dims[dir[0]] + nsets - 1) / nsets - start[dir[0]];
	}

	else {			/* Figure out partition based on cut directions. */
	    coords[0] = k & mask[0];
	    coords[1] = (k & mask[1]) >> nbits[0];
	    coords[2] = (k & mask[2]) >> (nbits[0] + nbits[1]);
	    if (snaking) {
		reverse = coords[1] & 1;
		if (reverse) coords[0] = mask[0] - coords[0];
		reverse = coords[2] & 1;
		if (reverse) coords[1] = (mask[1] >> nbits[0]) - coords[1];
	    }

	    for (j = 0; j < ndims; j++) {
		--nbits[dir[j]];
		if (coords[dir[j]] & (1 << nbits[dir[j]])) {
		    /* Right side of partition. */
		    start[dir[j]] += (width[dir[j]] + 1) / 2;
		    width[dir[j]] /= 2;
		}
		else {		/* Left side of partition */
		    width[dir[j]] = (width[dir[j]] + 1) / 2;
		}
	    }

	    /* Now restore nbits values. */
	    nbits[0] = nbits[1] = nbits[2] = 0;
	    for (i = 0; i < ndims; i++)
		++nbits[dir[i]];
	}


	for (i = 0; i < 3; i++)
	    start[i] += set->low[i];

	setnum = (start[2] * mesh_dims[1] + start[1]) * mesh_dims[0] + start[0];

	for (i = 0; i < 3; i++) {
	    set_info[setnum].low[i] = start[i];
	    set_info[setnum].span[i] = width[i];
	}
	subsets[k] = (short) setnum;
    }

    /* Check to see if hop_mtx is nonstandard. */
    flag = FALSE;
    if (KL_METRIC == 2) {
	if (*striping) {
	    flag = TRUE;
	    for (i = 0; i < nsets; i++) {
		for (j = 0; j < nsets; j++) {
		    hop_mtx_special[i][j] = abs(i - j);
		}
	    }
	}

	else if (nsets == 4) {
	    if (dir[0] == dir[1] || snaking) {
		flag = TRUE;
		for (i = 0; i < nsets; i++) {
		    start[0] = i & mask[0];
		    start[1] = (i & mask[1]) >> nbits[0];
		    if (snaking) {
			reverse = start[1] & 1;
			if (reverse) start[0] = mask[0] - start[0];
		    }
		    for (j = i; j < nsets; j++) {
			coords[0] = j & mask[0];
			coords[1] = (j & mask[1]) >> nbits[0];
			if (snaking) {
			    reverse = coords[1] & 1;
			    if (reverse) coords[0] = mask[0] - coords[0];
			}

			hop_mtx_special[i][j] = hop_mtx_special[j][i] =
			   abs(start[0] - coords[0]) + abs(start[1] - coords[1]);
		    }
		}
	    }
	}
	else if (nsets == 8) {
	    if (dir[0] == dir[1] || dir[0] == dir[2] || dir[1] == dir[2] || snaking) {
		flag = TRUE;
		for (i = 0; i < nsets; i++) {
		    start[0] = i & mask[0];
		    start[1] = (i & mask[1]) >> nbits[0];
		    start[2] = (i & mask[2]) >> (nbits[0] + nbits[1]);
		    if (snaking) {
			reverse = start[1] & 1;
			if (reverse) start[0] = mask[0] - start[0];
			reverse = start[2] & 1;
			if (reverse) start[1] = (mask[1] >> nbits[0]) - start[1];
		    }
		    for (j = i; j < nsets; j++) {
			coords[0] = j & mask[0];
			coords[1] = (j & mask[1]) >> nbits[0];
			coords[2] = (j & mask[2]) >> (nbits[0] + nbits[1]);
			if (snaking) {
			    reverse = coords[1] & 1;
			    if (reverse) coords[0] = mask[0] - coords[0];
			    reverse = coords[2] & 1;
			    if (reverse) coords[1] = (mask[1] >> nbits[0]) - coords[1];
			}

			hop_mtx_special[i][j] = hop_mtx_special[j][i] =
			   abs(start[0] - coords[0]) + abs(start[1] - coords[1]) +
			   abs(start[2] - coords[2]);
		    }
		}
	    }
	}
    }

    *striping |= snaking;

    return (flag);
}
