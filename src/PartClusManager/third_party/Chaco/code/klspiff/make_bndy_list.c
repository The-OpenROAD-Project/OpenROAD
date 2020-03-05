/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"structs.h"
#include	"defs.h"


void      make_bndy_list(graph, movelist, buckets, listspace, sets, nsets, bspace,
			 tops, bndy_list)
struct vtx_data **graph;	/* data structure for graph */
struct bilist *movelist;	/* list of vtxs to be moved */
struct bilist ****buckets;	/* array of lists for bucket sort */
struct bilist **listspace;	/* list data structure for each vertex */
short    *sets;			/* processor each vertex is assigned to */
int       nsets;		/* number of sets divided into */
int      *bspace;		/* list of active vertices for bucketsort */
int     **tops;			/* top of each set of buckets */
int     **bndy_list;		/* list of boundary vertices returned */
{
    struct bilist *bptr;	/* loops through bspace */
    int       vtx;		/* vertex that was moved */
    int       set;		/* set a vertex is in */
    int       list_length;	/* number of active vertices */
    int       bndy_length;	/* number of vertices actually on boundary */
    int       size;		/* array spacing */
    int       i, j, k;		/* loop counters */
    double   *smalloc();

    /* First push all the moved vertices onto list, so they can be flagged. */
    /* They've already been removed from buckets, so want to avoid them. */
    size = (int) (&(listspace[0][1]) - &(listspace[0][0]));
    bptr = movelist;
    list_length = 0;
    while (bptr != NULL) {
	vtx = ((int) (bptr - listspace[0])) / size;
	bspace[list_length++] = vtx;
	bptr = bptr->next;
    }

    /* Now get all the vertices still in the bucket lists. */
    for (k=tops[0][1]; k >= 0; k--) {
	bptr = buckets[0][1][k];
	while (bptr != NULL) {
	    vtx = ((int) (bptr - listspace[0])) / size;
	    bspace[list_length++] = vtx;
	    bptr = bptr->next;
	}
    }

    for (i=1; i<nsets; i++) {
        for (k=tops[i][0]; k >= 0; k--) {
	    bptr = buckets[i][0][k];
	    while (bptr != NULL) {
	        vtx = ((int) (bptr - listspace[0])) / size;
	        bspace[list_length++] = vtx;
	        bptr = bptr->next;
	    }
	}
    }


    /* Now that list is constructed, go reconstruct all the set numbers. */
    bndy_length = 0;
    for (i = 0; i< list_length; i++) {
	vtx = bspace[i];
	set = sets[vtx];
	for (j=1; j<graph[vtx]->nedges; j++) {
	    if (sets[graph[vtx]->edges[j]] != set) {
		bspace[bndy_length++] = vtx;
		break;
		
	    }
	}
    }

    /* Finally, copy boundary vertices into boundary list. */
    *bndy_list = (int *) smalloc((unsigned) (bndy_length + 1) * sizeof(int));
    for (i=0; i<bndy_length; i++) {
	(*bndy_list)[i] = bspace[i];
    }
    (*bndy_list)[bndy_length] = 0;
}
