/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"params.h"
#include	"structs.h"
#include	"defs.h"

/* Idea:
   'buckets[i][j]' is a set of buckets to sort moves from i to j.
   listspace[i] is space for lists in buckets[i][j].
   Loop through all nonequal pairs [i][j], taking the first element
   in each list.  Compare them all to find the largest allowed move.
   Make that move, and save it in movelist.
*/


void      bucketsort1(graph, vtx, buckets, listspace, dvals, sets, term_wgts,
             maxdval, nsets, hops,  using_ewgts)
struct vtx_data **graph;	/* graph data structure */
int       vtx;			/* vertex being added to lists */
struct bilist ****buckets;	/* array of lists for bucket sort */
struct bilist **listspace;	/* list data structure for each vertex */
int     **dvals;		/* d-values for each vertex for removing */
short    *sets;			/* processor each vertex is assigned to */
float    *term_wgts[];		/* weights for terminal propogation */
int       maxdval;		/* maximum possible dvalue for a vertex */
int       nsets;		/* number of sets being divided into */
short     (*hops)[MAXSETS];	/* hop cost between sets */
int       using_ewgts;		/* are edge weights being used? */
{
    extern double CUT_TO_HOP_COST;	/* if term_prop, cut/hop importance */
    struct bilist *lptr;	/* pointer to an element in listspace */
    float    *ewptr;		/* loops through edge weights */
    int      *edges;		/* edge list for a vertex */
    int       myset;		/* set that current vertex belongs to */
    int       newset;		/* set current vertex could move to */
    int       set;		/* set that neighboring vertex belongs to */
    int       weight;		/* edge weight for a particular edge */
    float     tval;		/* terminal propagation value */
    int       val;		/* terminal propogation rounded value */
    double    cut_cost;		/* relative cut/hop importance */
    double    hop_cost;		/* relative hop/cut importance */
    int       myhop;		/* hops associated with current vertex */
    int       j, l;		/* loop counters */
    void      add2bilist();


    /* Compute d-vals by seeing which sets neighbors belong to. */
    cut_cost = hop_cost = 1;
    if (term_wgts[1] != NULL) {
	if (CUT_TO_HOP_COST > 1) {
	    cut_cost = CUT_TO_HOP_COST;
	}
	else {
	    hop_cost = 1.0 / CUT_TO_HOP_COST;
	}
    }

    weight = cut_cost + .5;
    myset = sets[vtx];

    /* Initialize all the preference values. */
    if (term_wgts[1] != NULL) {
	/* Using terminal propogation. */
	if (myset == 0) {	/* No terminal value. */
	    for (newset = 1; newset < nsets; newset++) {
		tval = (term_wgts[newset])[vtx];
		if (tval < 0) {
		    val = - tval * hop_cost + .5;
		    val = - val;
		}
		else {
		    val = tval * hop_cost + .5;
		}
		dvals[vtx][newset - 1] = val;
	    }
	}
	else {
	    tval = -(term_wgts[myset])[vtx];
	    if (tval < 0) {
		val = - tval * hop_cost + .5;
		val = - val;
	    }
	    else {
		val = tval * hop_cost + .5;
	    }
	    dvals[vtx][0] = val;
	    l = 1;
	    for (newset = 1; newset < nsets; newset++) {
		if (newset != myset) {
		    tval = (term_wgts[newset])[vtx] - (term_wgts[myset])[vtx];
		    if (tval < 0) {
		        val = - tval * hop_cost + .5;
		        val = - val;
		    }
		    else {
		        val = tval * hop_cost + .5;
		    }
		    dvals[vtx][l] = val;
		    l++;
		}
	    }
	}
    }
    else {
	for (j = 0; j < nsets - 1; j++)
	    dvals[vtx][j] = 0;
    }

    /* First count the neighbors in each set. */
    edges = graph[vtx]->edges;
    if (using_ewgts)
	ewptr = graph[vtx]->ewgts;
    for (j = graph[vtx]->nedges - 1; j; j--) {
	set = sets[*(++edges)];
	if (set < 0)
	    set = -set - 1;
	if (using_ewgts)
	    weight = *(++ewptr) * cut_cost + .5;
	myhop = hops[myset][set];

	l = 0;
	for (newset = 0; newset < nsets; newset++) {
	    if (newset != myset) {
		dvals[vtx][l] += weight * (myhop - hops[newset][set]);
		l++;
	    }
	}
    }

    /* Now add to appropriate buckets. */
    l = 0;
    for (newset = 0; newset < nsets; newset++) {
	if (newset != myset) {
	    lptr = listspace[l];
	    add2bilist(&lptr[vtx], &buckets[myset][newset][dvals[vtx][l] + maxdval]);
	    ++l;
	}
    }
}
