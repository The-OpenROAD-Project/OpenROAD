/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

/* Combine the old assignment value with the new partition. */
void      merge_assignments(assignment, subassign, subsets, subnvtxs, loc2glob)
short    *assignment;		/* assignment list for graph */
short    *subassign;		/* subgraph assignment list */
short    *subsets;		/* mapping from local to global sets */
int       subnvtxs;		/* number of vtxs in subgraph */
int      *loc2glob;		/* subgraph -> graph numbering map */
{
    int       i;		/* loop counter */

    for (i = 1; i <= subnvtxs; i++) {
	assignment[loc2glob[i]] = subsets[subassign[i]];
    }
}
