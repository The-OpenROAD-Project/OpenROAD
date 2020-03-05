/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"structs.h"
#include	"defs.h"

/* Refine a vertex separator by finding a maximum bipartite matching. */

static int bpm_improve1();
static double sep_cost();

void      bpm_improve(graph, sets, goal, max_dev, bndy_list, weights, using_vwgts)
struct vtx_data **graph;	/* list of graph info for each vertex */
short    *sets;			/* local partitioning of vtxs */
double   *goal;			/* desired set sizes */
int       max_dev;		/* largest deviation from balance allowed */
int     **bndy_list;		/* list of vertices on boundary (0 ends) */
double   *weights;		/* vertex weights in each set */
int       using_vwgts;		/* invoke weighted cover routines? */
{
    extern int DEBUG_COVER;	/* debug flag for min vertex cover */
    extern int VERTEX_COVER;	/* apply improvement once, or repeatedly? */
    double    ratio;		/* fraction of non-separator vertices */
    double    deltaplus;	/* amount set is too big */
    double    deltaminus;	/* amount set is too small */
    double    imbalance;	/* current amount of imbalance */
    int       set_big;		/* side of graph I'm matching against */
    int       set_small;	/* side of graph I'm not matching against */
    int       sep_size;		/* separator size */
    int       sep_weight;	/* weight of separator */
    int       change;		/* does separator get improved? */
    int       i;		/* loop counter */
    double    old_cost;
    double    fabs();

    sep_size = 0;
    while ((*bndy_list)[sep_size] != 0) {
	sep_size++;
    }
    if (using_vwgts) {
	sep_weight = 0;
	for (i = 0; i < sep_size; i++) {
	    sep_weight += graph[(*bndy_list)[i]]->vwgt;
	}
    }
    else {
	sep_weight = sep_size;
    }

    if (DEBUG_COVER > 1) {
	printf("Before first matching, sep_size = %d, sep_weight = %d,  Sizes = %g/%g\n",
	       sep_size, sep_weight, weights[0], weights[1]);
    }

    ratio = (weights[0] + weights[1]) / (goal[0] + goal[1]);
    deltaplus = fabs(weights[0] - goal[0] * ratio);
    deltaminus = fabs(weights[1] - goal[1] * ratio);
    imbalance = deltaplus + deltaminus;
    old_cost = sep_cost(weights[0], weights[1], (double) sep_weight, (double) max_dev);


    change = TRUE;
    while (change) {
	/* First match towards the larger side, then the smaller. */
	if (goal[0] - weights[0] >= goal[1] - weights[1]) {
	    set_big = 1;
	    set_small = 0;
	}
	else {
	    set_big = 0;
	    set_small = 1;
	}

	change = bpm_improve1(graph, sets, bndy_list, weights, set_big, set_small,
			      goal, max_dev, &imbalance, &sep_size, &sep_weight,
			      using_vwgts, &old_cost);

	if (DEBUG_COVER) {
	    printf("After big matching, sep_size = %d, sep_weight = %d,  Sizes = %g/%g\n",
		   sep_size, sep_weight, weights[0], weights[1]);
	}
	if (VERTEX_COVER == 1)
	    break;

	if (!change) {
	    /* If balanced, try the other direction. */
	    if (imbalance < max_dev) {
		change = bpm_improve1(graph, sets, bndy_list, weights, set_small,
			set_big, goal, max_dev, &imbalance, &sep_size, &sep_weight,
				      using_vwgts, &old_cost);

		if (DEBUG_COVER) {
		    printf("After small matching, sep_size = %d,  Sizes = %g/%g\n",
			   sep_size, weights[0], weights[1]);
		}
	    }
	}
    }
    if (DEBUG_COVER) {
	printf("After all matchings, sep_size = %d, sep_weight = %d,  Sizes = %g/%g\n\n",
	       sep_size, sep_weight, weights[0], weights[1]);
    }
}

static int bpm_improve1(graph, sets, pbndy_list, weights, set_match, set_other,
	              goal, max_dev, pimbalance, sep_size, sep_weight, using_vwgts,
			          pcost)
struct vtx_data **graph;	/* list of graph info for each vertex */
short    *sets;			/* local partitioning of vtxs */
int     **pbndy_list;		/* list of vertices on boundary (0 ends) */
double   *weights;		/* vertex weights in each set */
int       set_match;		/* side of graph I'm matching against */
int       set_other;		/* side of graph I'm not matching against */
double   *goal;			/* desired set sizes */
int       max_dev;		/* largest deviation from balance allowed */
double   *pimbalance;		/* imbalance of current partition */
int      *sep_size;		/* separator size */
int      *sep_weight;		/* weight of separator */
int       using_vwgts;		/* use weighted model? */
double   *pcost;		/* cost of current separator */
{
    extern int DEBUG_COVER;	/* debug flag for min vertex cover */
    double    new_weights[2];	/* weights associated with new separator */
    double    ratio;		/* fraction of non-separator vertices */
    double    deltaplus;	/* amount set is too big */
    double    deltaminus;	/* amount set is too small */
    double    new_imbalance;	/* imbalance of new partition */
    double    new_cost;		/* cost of new separator */
    int      *pointers;		/* start/stop indices into adjacencies */
    int      *indices;		/* adjacencies for each bipartite vertex */
    int      *vweight;		/* vertex weights if needed */
    int      *loc2glob;		/* mapping from bp graph to original */
    int      *new_bndy_list;	/* new list of boundary vertices */
    int       old_sep_size;	/* previous separator size */
    int       old_sep_weight;	/* previous separator weight */
    int       vtx;		/* vertex in graph */
    int       change;		/* does this routine alter separator? */
    int       nleft, nright;	/* # vtxs in two sides on bp graph */
    int       i, j;		/* loop counter */
    void      make_bpgraph(), bpcover(), wbpcover();
    double   *smalloc(), fabs();
    int       sfree();

    make_bpgraph(graph, sets, *pbndy_list, *sep_size, set_match, &pointers,
		 &indices, &vweight, &loc2glob, &nleft, &nright, using_vwgts);

    old_sep_size = *sep_size;
    old_sep_weight = *sep_weight;
    if (!using_vwgts) {
	new_bndy_list = (int *) smalloc((unsigned) (*sep_size + 1) * sizeof(int));
	new_bndy_list[0] = nleft + nright;
	bpcover(nleft, nright, pointers, indices, sep_size, new_bndy_list);
	*sep_weight = *sep_size;
    }
    else {
	wbpcover(nleft, nright, pointers, indices, vweight, sep_size,
		 sep_weight, &new_bndy_list);
    }

    /* Update weights. */
    new_weights[0] = weights[0];
    new_weights[1] = weights[1];
    for (j = 0; j < new_bndy_list[0]; j++) {
	/* First handle nodes numbered less than separator nodes. */
	vtx = loc2glob[j];
	if (sets[vtx] == 2) {
	    new_weights[set_other] += graph[vtx]->vwgt;
	}
    }
    for (i = 0; i < *sep_size; i++) {
	vtx = loc2glob[new_bndy_list[i]];
	if (sets[vtx] == set_match) {
	    new_weights[set_match] -= graph[vtx]->vwgt;
	}
	if (i != 0) {
	    for (j = new_bndy_list[i - 1] + 1; j < new_bndy_list[i]; j++) {
		vtx = loc2glob[j];
		if (sets[vtx] == 2) {
		    new_weights[set_other] += graph[vtx]->vwgt;
		}
	    }
	}
    }
    if (*sep_size != 0) {
	i = new_bndy_list[*sep_size - 1] + 1;
    }
    else {
	i = 0;
    }
    for (j = i; j < nleft + nright; j++) {
	vtx = loc2glob[j];
	if (sets[vtx] == 2) {
	    new_weights[set_other] += graph[vtx]->vwgt;
	}
    }


    /* Check to see if new partition is acceptably balanced. */
    ratio = (new_weights[0] + new_weights[1]) / (goal[0] + goal[1]);
    deltaplus = fabs(new_weights[0] - goal[0] * ratio);
    deltaminus = fabs(new_weights[1] - goal[1] * ratio);
    new_imbalance = deltaplus + deltaminus;

    new_cost = sep_cost(weights[0], weights[1], (double) *sep_weight, (double) max_dev);

    if (DEBUG_COVER > 1) {
	printf("Sides %.0f, %.0f: sep %d total %.0f %.0f\n",
	       new_weights[0], new_weights[1], *sep_size,
	       new_weights[0] + new_weights[1],
	       new_weights[0] + new_weights[1] + *sep_size
	   );
    }

    /* if (new_cost < *pcost) { */
    if ((new_cost < *pcost && new_imbalance <= max_dev) ||
	    (new_cost <= *pcost && new_imbalance < *pimbalance)) {
	/* Update set values. */
	change = TRUE;
	*pcost = new_cost;
	for (j = 0; j < new_bndy_list[0]; j++) {
	    /* First handle nodes numbered  less than separator nodes. */
	    vtx = loc2glob[j];
	    if (sets[vtx] == 2) {
		sets[vtx] = (short) set_other;
	    }
	}
	for (i = 0; i < *sep_size; i++) {
	    vtx = loc2glob[new_bndy_list[i]];
	    if (sets[vtx] == set_match) {
		sets[vtx] = 2;
	    }
	    if (i != 0) {
		for (j = new_bndy_list[i - 1] + 1; j < new_bndy_list[i]; j++) {
		    vtx = loc2glob[j];
		    if (sets[vtx] == 2) {
			sets[vtx] = (short) set_other;
		    }
		}
	    }
	}
	if (*sep_size != 0) {
	    i = new_bndy_list[*sep_size - 1] + 1;
	}
	else {
	    i = 0;
	}
	for (j = i; j < nleft + nright; j++) {
	    vtx = loc2glob[j];
	    if (sets[vtx] == 2) {
		sets[vtx] = (short) set_other;
	    }
	}

	/* Restore bndy_list to global numbering. */
	for (i = 0; i < *sep_size; i++) {
	    new_bndy_list[i] = loc2glob[new_bndy_list[i]];
	}

	new_bndy_list[*sep_size] = 0;

	sfree((char *) *pbndy_list);

	*pbndy_list = new_bndy_list;
	*pimbalance = new_imbalance;

	weights[0] = new_weights[0];
	weights[1] = new_weights[1];
    }

    else {
	change = FALSE;
	sfree((char *) new_bndy_list);
	*sep_size = old_sep_size;
	*sep_weight = old_sep_weight;
    }

    sfree((char *) vweight);
    sfree((char *) loc2glob);
    sfree((char *) indices);
    sfree((char *) pointers);

    return (change);
}


/* Routine that can be modified to allow different cost functions. */

static double sep_cost(size1, size2, size_sep, max_dev)
double    size1, size2;		/* vertex weight in two partitions */
double    size_sep;		/* vertex weight of separator */
double    max_dev;		/* maximum allowed imbalance */
{
    return ((double) size_sep);
}
