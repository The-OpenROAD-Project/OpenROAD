/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include "defs.h"
#include "structs.h"
#include "params.h"


int       define_subcubes(nsets_real, ndims_tot, ndims, set, set_info, subsets,
			            inert, pstriping, hop_mtx_special)
int       nsets_real;		/* actual number of sets being created */
int       ndims_tot;		/* total hypercube dimensions */
int       ndims;		/* # dimension in this cut */
struct set_info *set;		/* data for set being divided */
struct set_info *set_info;	/* data for all sets */
short    *subsets;		/* subsets to be created */
int       inert;		/* using inertial method? */
int      *pstriping;		/* cut in single direction? */
short     hop_mtx_special[MAXSETS][MAXSETS];	/* nonstandard hop values */
{
    extern int KL_METRIC;	/* 2 => using hops so generate hop matrix */
    int       hop_flag;		/* use special hop matrix? */
    int       nsets;		/* number of sets being created */
    int       setnum;		/* global number of subset */
    int       bits;		/* number of bits in which two sets differ */
    int       i, j, k;		/* loop counters */
    int       gray();

    nsets = 1 << ndims;
    hop_flag = FALSE;

    for (k = nsets - 1; k >= 0; k--) {	/* Backwards to not overwrite current set. */

	setnum = set->setnum | (k << (ndims_tot - set->ndims));
	set_info[setnum].ndims = set->ndims - ndims;
	subsets[k] = (short) setnum;
    }

    *pstriping = (inert && nsets_real > 2);

    if (*pstriping) {		/* Gray code for better mapping. */
        for (k = 0; k < nsets; k++) {
	    subsets[k] = (short) gray((int) subsets[k]);
	}

	if (KL_METRIC == 2) {
	    hop_flag = TRUE;
	    for (i = 0; i < nsets; i++) {
		hop_mtx_special[i][i] = 0;
		for (j = 0; j < i; j++) {
		    hop_mtx_special[i][j] = 0;
		    bits = ((int) subsets[i]) ^ ((int) subsets[j]);
		    while (bits) {
			if (bits & 1) {
			    ++hop_mtx_special[i][j];
			}
			bits >>= 1;
		    }
		    hop_mtx_special[j][i] = hop_mtx_special[i][j];
		}
	    }
	}
    }

    return(hop_flag);
}
