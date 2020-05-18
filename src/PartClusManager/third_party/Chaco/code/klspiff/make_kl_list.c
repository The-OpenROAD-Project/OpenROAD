/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"structs.h"
#include	"defs.h"


int       make_kl_list(graph, movelist, buckets, listspace, sets, nsets, bspace, dvals,
		                 maxdval)
struct vtx_data **graph;	/* data structure for graph */
struct bilist *movelist;	/* list of vtxs to be moved */
struct bilist ****buckets;	/* array of lists for bucket sort */
struct bilist **listspace;	/* list data structure for each vertex */
short    *sets;			/* processor each vertex is assigned to */
int       nsets;		/* number of sets divided into */
int      *bspace;		/* list of active vertices for bucketsort */
int     **dvals;		/* d-values for each transition */
int       maxdval;		/* maximum d-value for a vertex */
{
    struct bilist **list;	/* bucket to erase element from */
    struct bilist *vptr;	/* loops through movelist */
    int      *bptr;		/* loops through bspace */
    int      *iptr;		/* loops through edge list */
    int       vtx;		/* vertex that was moved */
    int       neighbor;		/* neighbor of a vertex */
    int       myset;		/* set a vertex is in */
    int       newset;		/* loops through other sets */
    int       list_length;	/* number of values put in bspace */
    int       size;		/* array spacing */
    int       i, l;		/* loop counter */
    void      removebilist();

    /* First push all the moved vertices onto list, so they can be flagged. */
    /* They've already been removed from buckets, so want to avoid them. */
    size = (int) (&(listspace[0][1]) - &(listspace[0][0]));
    vptr = movelist;
    bptr = bspace;
    list_length = 0;
    while (vptr != NULL) {
	vtx = ((int) (vptr - listspace[0])) / size;
	*bptr++ = vtx;
	if (sets[vtx] >= 0)
	    sets[vtx] = -sets[vtx] - 1;
	++list_length;
	vptr = vptr->next;
    }

    /* Now look at all the neighbors of moved vertices. */
    vptr = movelist;
    while (vptr != NULL) {
	vtx = ((int) (vptr - listspace[0])) / size;

	iptr = graph[vtx]->edges;
	for (i = graph[vtx]->nedges - 1; i; i--) {
	    neighbor = *(++iptr);
	    if (sets[neighbor] >= 0) {
		*bptr++ = neighbor;
		++list_length;
		myset = sets[neighbor];
		sets[neighbor] = -sets[neighbor] - 1;

		/* Remove neighbor entry from all his buckets. */
		/* Note: vertices in movelist already removed from buckets. */
		l = 0;
		for (newset = 0; newset < nsets; newset++) {
		    if (newset != myset) {
			list = &buckets[myset][newset][dvals[neighbor][l] + maxdval];
			removebilist(&listspace[l][neighbor], list);
			l++;
		    }
		}
	    }
	}
	vptr = vptr->next;
    }

    /* Now that list is constructed, go reconstruct all the set numbers. */
    bptr = bspace;
    for (i = list_length; i; i--) {
	vtx = *bptr++;
	sets[vtx] = -sets[vtx] - 1;
    }

    return (list_length);
}
