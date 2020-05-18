/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"structs.h"
#include	"defs.h"


int       kl_init(bucket_ptrs, listspace, dvals, tops, nvtxs, nsets, maxchange)
struct bilist *****bucket_ptrs;	/* space for multiple bucket sorts */
struct bilist ***listspace;	/* space for all elements of linked lists */
int    ***dvals;		/* change in cross edges for each move */
int    ***tops;			/* top dval for each type of move */
int       nvtxs;		/* number of vertices in the graph */
int       nsets;		/* number of sets created at each step */
int       maxchange;		/* maximum change by moving a vertex */
{
    struct bilist *spacel;	/* space for all listspace entries */
    struct bilist **spaceb;	/* space for all buckets entries */
    int       sizeb;		/* size of set of buckets */
    int       sizel;		/* size of set of pointers for all vertices */
    int       i, j;		/* loop counters */
    double   *array_alloc_2D_ret();
    double   *smalloc_ret();

    /* Allocate appropriate data structures for buckets, and listspace. */

    *bucket_ptrs = (struct bilist ****)
       array_alloc_2D_ret(nsets, nsets, sizeof(struct bilist *));

    *dvals = (int **) array_alloc_2D_ret(nvtxs + 1, nsets - 1, sizeof(int));

    *tops = (int **) array_alloc_2D_ret(nsets, nsets, sizeof(int));

    /* By using '-1' in the next line, I save space, but I need to */
    /* be careful to get the right element in listspace each time. */
    *listspace = (struct bilist **) smalloc_ret((unsigned) (nsets - 1) * sizeof(struct bilist *));

    sizeb = (2 * maxchange + 1) * sizeof(struct bilist *);
    sizel = (nvtxs + 1) * sizeof(struct bilist);
    spacel = (struct bilist *) smalloc_ret((unsigned) (nsets - 1) * sizel);
    spaceb = (struct bilist **) smalloc_ret((unsigned) nsets * (nsets - 1) * sizeb);

    if (*bucket_ptrs == NULL || *dvals == NULL || *tops == NULL ||
	*listspace == NULL || spacel == NULL || spaceb == NULL) {
	return(1);
    }

    for (i = 0; i < nsets; i++) {
	if (i != nsets - 1) {
	    (*listspace)[i] = spacel;
	    spacel += nvtxs + 1;
	}

	for (j = 0; j < nsets; j++) {
	    if (i != j) {
		(*bucket_ptrs)[i][j] = spaceb;
		spaceb += 2 * maxchange + 1;
	    }
	}
    }

    return(0);
}
