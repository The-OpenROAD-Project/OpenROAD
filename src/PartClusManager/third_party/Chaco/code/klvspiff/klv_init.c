/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>
#include	"structs.h"
#include	"defs.h"


int       klv_init(lbucket_ptr, rbucket_ptr, llistspace, rlistspace,
		   ldvals, rdvals, nvtxs, maxchange)
struct bilist ***lbucket_ptr;	/* space for left bucket sorts */
struct bilist ***rbucket_ptr;	/* space for right bucket sorts */
struct bilist **llistspace;	/* space for elements of linked lists */
struct bilist **rlistspace;	/* space for elements of linked lists */
int     **ldvals;		/* change in separator for left moves */
int     **rdvals;		/* change in separator for right moves */
int       nvtxs;		/* number of vertices in the graph */
int       maxchange;		/* maximum change by moving a vertex */
{
    int       sizeb;		/* size of set of buckets */
    int       sizel;		/* size of set of pointers for all vertices */
    int       flag;		/* return code */
    double   *smalloc_ret();

    /* Allocate appropriate data structures for buckets, and listspace. */

    sizeb = (2 * maxchange + 1) * sizeof(struct bilist *);
    *lbucket_ptr = (struct bilist **) smalloc_ret((unsigned) sizeb);
    *rbucket_ptr = (struct bilist **) smalloc_ret((unsigned) sizeb);

    *ldvals = (int *) smalloc_ret((unsigned) (nvtxs + 1) * sizeof(int));
    *rdvals = (int *) smalloc_ret((unsigned) (nvtxs + 1) * sizeof(int));

    sizel = (nvtxs + 1) * sizeof(struct bilist);
    *llistspace = (struct bilist *) smalloc_ret((unsigned) sizel);
    *rlistspace = (struct bilist *) smalloc_ret((unsigned) sizel);

    if (*lbucket_ptr == NULL || *rbucket_ptr == NULL || *ldvals == NULL ||
	*rdvals == NULL || *llistspace == NULL || *rlistspace == NULL) {
	flag = 1;
    }

    else {
        flag = 0;
    }

    return(flag);
}
